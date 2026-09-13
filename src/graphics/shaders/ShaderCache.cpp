#include "graphics/shaders/ShaderCache.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "core/FileIo.hpp"
#include "core/Logger.hpp"
#include "graphics/shaders/TechniqueRegistry.hpp"

namespace gfx {
namespace {

using json = nlohmann::json;

constexpr uint64_t k_fnv_offset = 14695981039346656037ull;
constexpr uint64_t k_fnv_prime = 1099511628211ull;

void fnv1a_update(uint64_t& hash, const void* data, size_t size) {
  const auto* bytes = static_cast<const uint8_t*>(data);
  for (size_t i = 0; i < size; ++i) {
    hash ^= bytes[i];
    hash *= k_fnv_prime;
  }
}

void fnv1a_update(uint64_t& hash, std::string_view text) {
  fnv1a_update(hash, text.data(), text.size());
  // Field separator so "ab"+"c" != "a"+"bc"
  const char sep = '\0';
  fnv1a_update(hash, &sep, 1);
}

std::string hash_to_hex(uint64_t hash) {
  static constexpr char k_hex[] = "0123456789abcdef";
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i) {
    out[static_cast<size_t>(i)] = k_hex[hash & 0xfu];
    hash >>= 4;
  }
  return out;
}

const char* stage_name(rhi::ShaderType stage) {
  switch (stage) {
    case rhi::ShaderType::Vertex:
      return "vertex";
    case rhi::ShaderType::Fragment:
      return "fragment";
    case rhi::ShaderType::Compute:
      return "compute";
    case rhi::ShaderType::Mesh:
      return "mesh";
    case rhi::ShaderType::Task:
      return "task";
    case rhi::ShaderType::None:
      break;
  }
  return "none";
}

// TODO: use newer profile if possible
const char* profile_for_stage(rhi::ShaderType stage) {
  switch (stage) {
    case rhi::ShaderType::Vertex:
      return "vs_6_6";
    case rhi::ShaderType::Fragment:
      return "ps_6_6";
    case rhi::ShaderType::Compute:
      return "cs_6_6";
    case rhi::ShaderType::Mesh:
      return "ms_6_6";
    case rhi::ShaderType::Task:
      return "as_6_6";
    case rhi::ShaderType::None:
      break;
  }
  return "vs_6_6";
}

/// One compiled stage of a technique (cook unit).
struct CacheUnit {
  std::string_view technique_name;
  std::filesystem::path source_path;
  rhi::ShaderType stage{rhi::ShaderType::Vertex};
  std::string entry;
  std::vector<std::string> defines;
};

struct MetaFile {
  std::string content_hash;
  std::string entry;
  std::string stage;
  std::string profile;
  std::vector<std::string> defines;
  std::vector<std::string> dependencies;
  std::string dxc_version;
  std::string metal_ir_converter_version;
};

bool load_meta(const std::filesystem::path& path, MetaFile& meta, std::string& error) {
  std::vector<uint8_t> bytes;
  if (!core::read_bytes(path, bytes, error)) {
    return false;
  }
  try {
    const json j = json::parse(bytes.begin(), bytes.end());
    meta.content_hash = j.at("content_hash").get<std::string>();
    meta.entry = j.at("entry").get<std::string>();
    meta.stage = j.at("stage").get<std::string>();
    meta.profile = j.at("profile").get<std::string>();
    meta.dxc_version = j.at("dxc_version").get<std::string>();
    meta.metal_ir_converter_version = j.at("metal_ir_converter_version").get<std::string>();
    meta.defines = j.value("defines", std::vector<std::string>{});
    meta.dependencies = j.value("dependencies", std::vector<std::string>{});
  } catch (const json::exception& ex) {
    error = std::string("Malformed shader meta: ") + path.string() + " (" + ex.what() + ")";
    return false;
  }
  return true;
}

bool save_meta(const std::filesystem::path& path, const MetaFile& meta, std::string& error) {
  const json j = {
      {"content_hash", meta.content_hash},
      {"entry", meta.entry},
      {"stage", meta.stage},
      {"profile", meta.profile},
      {"defines", meta.defines},
      {"dependencies", meta.dependencies},
      {"dxc_version", meta.dxc_version},
      {"metal_ir_converter_version", meta.metal_ir_converter_version},
  };
  const std::string text = j.dump(2) + '\n';
  return core::write_bytes(path, text.data(), text.size(), error);
}

std::vector<std::string> sorted_copy(std::vector<std::string> values) {
  std::ranges::sort(values);
  return values;
}

std::string compute_content_hash(const CacheUnit& unit,
                                 const std::vector<std::string>& dependencies,
                                 std::string_view dxc_version, std::string_view metal_ir_version) {
  uint64_t hash = k_fnv_offset;
  fnv1a_update(hash, profile_for_stage(unit.stage));
  fnv1a_update(hash, unit.entry);
  fnv1a_update(hash, stage_name(unit.stage));
  for (const auto& define : sorted_copy(unit.defines)) {
    fnv1a_update(hash, define);
  }
  fnv1a_update(hash, dxc_version);
  fnv1a_update(hash, metal_ir_version);

  auto deps = sorted_copy(dependencies);
  for (const auto& dep : deps) {
    fnv1a_update(hash, dep);
    std::vector<uint8_t> bytes;
    std::string error;
    if (!core::read_bytes(dep, bytes, error)) {
      // Missing dep → unstable hash, forces rebuild next time if it reappears.
      fnv1a_update(hash, std::string_view{"<missing>"});
      continue;
    }
    fnv1a_update(hash, bytes.data(), bytes.size());
  }
  return hash_to_hex(hash);
}

std::filesystem::path unit_stem(const CacheUnit& unit) {
  return std::filesystem::path{std::string(unit.technique_name)} / unit.entry;
}

std::filesystem::path metallib_path(const ShaderCache::CacheRoots& roots, const CacheUnit& unit) {
  auto path = roots.cache_root / "metal" / unit_stem(unit);
  path += ".metallib";
  return path;
}

std::filesystem::path dxil_path(const ShaderCache::CacheRoots& roots, const CacheUnit& unit) {
  auto path = roots.cache_root / "metal" / unit_stem(unit);
  path += ".dxil";
  return path;
}

std::filesystem::path meta_path(const ShaderCache::CacheRoots& roots, const CacheUnit& unit) {
  auto path = roots.cache_root / "meta" / unit_stem(unit);
  path += ".json";
  return path;
}

bool make_unit(const ShaderCache::CacheRoots& roots, const ShaderTechniqueDesc& tech,
               const ShaderTechniqueDesc::StageDesc& stage, CacheUnit& out, std::string* error) {
  if (roots.shader_root.empty()) {
    if (error) {
      *error = "ShaderCache shader_root is empty";
    }
    return false;
  }
  out.technique_name = tech.name;
  out.source_path = roots.shader_root / std::string(tech.path);
  out.stage = stage.stage;
  out.entry = std::string(stage.entry);
  out.defines.clear();
  out.defines.reserve(tech.defines.size());
  for (std::string_view define : tech.defines) {
    out.defines.emplace_back(define);
  }
  return true;
}

bool make_unit(const ShaderCache::CacheRoots& roots, std::string_view technique_name,
               rhi::ShaderType stage, CacheUnit& out, std::string* error) {
  const ShaderTechniqueDesc* tech = ShaderTechniqueRegistry::find(technique_name);
  if (!tech) {
    if (error) {
      *error = "Unknown technique: " + std::string(technique_name);
    }
    return false;
  }
  for (const ShaderTechniqueDesc::StageDesc& stage_desc : tech->stages) {
    if (stage_desc.stage == stage) {
      return make_unit(roots, *tech, stage_desc, out, error);
    }
  }
  if (error) {
    *error = "Technique '" + std::string(technique_name) + "' has no stage " + stage_name(stage);
  }
  return false;
}

bool unit_is_outdated(const ShaderCache::CacheRoots& roots, const CacheUnit& unit,
                      std::string* reason) {
  const auto metallib = metallib_path(roots, unit);
  const auto meta = meta_path(roots, unit);
  std::error_code ec;
  if (!std::filesystem::is_regular_file(metallib, ec)) {
    if (reason) {
      *reason = "missing metallib";
    }
    return true;
  }
  // Packaged builds may omit meta/ → treat existing metallib as prebuilt (up to date).
  if (!std::filesystem::is_regular_file(meta, ec)) {
    return false;
  }

  MetaFile parsed_meta;
  std::string error;
  if (!load_meta(meta, parsed_meta, error)) {
    if (reason) {
      *reason = error;
    }
    return true;
  }

  if (parsed_meta.entry != unit.entry || parsed_meta.stage != stage_name(unit.stage) ||
      parsed_meta.profile != profile_for_stage(unit.stage) ||
      sorted_copy(parsed_meta.defines) != sorted_copy(unit.defines)) {
    if (reason) {
      *reason = "entry/stage/defines mismatch";
    }
    return true;
  }

  const ShaderToolVersions tools = query_shader_tool_versions();
  if (!tools.ok()) {
    if (reason) {
      *reason = tools.error_message;
    }
    return true;
  }
  if (tools.dxc != parsed_meta.dxc_version ||
      tools.metal_ir_converter != parsed_meta.metal_ir_converter_version) {
    if (reason) {
      *reason = "tool version mismatch";
    }
    return true;
  }

  bool has_source = false;
  const std::string source_str =
      std::filesystem::absolute(unit.source_path, ec).lexically_normal().string();
  for (const auto& dep : parsed_meta.dependencies) {
    if (std::filesystem::path{dep}.lexically_normal().string() == source_str) {
      has_source = true;
      break;
    }
  }
  if (!has_source) {
    if (reason) {
      *reason = "meta missing primary source dependency";
    }
    return true;
  }

  const std::string hash =
      compute_content_hash(unit, parsed_meta.dependencies, tools.dxc, tools.metal_ir_converter);
  if (hash != parsed_meta.content_hash) {
    if (reason) {
      *reason = "content hash mismatch";
    }
    return true;
  }

  return false;
}

bool ensure_unit(const ShaderCache::CacheRoots& roots, const CacheUnit& unit, bool force,
                 std::string* error) {
  if (!force) {
    std::string reason;
    if (!unit_is_outdated(roots, unit, &reason)) {
      return true;
    }
    LINFO("shader outdated ({}): {} / {}", reason, unit.technique_name, unit.entry);
  }

  ShaderCompileInput input{
      .source_path = unit.source_path,
      .stage = unit.stage,
      .entry_point = unit.entry,
      .include_directories =
          {
              (roots.shader_root / "include").string(),
              unit.source_path.parent_path().string(),
          },
      .defines = unit.defines,
  };

  ShaderCompileOutput output;
  if (!compile(input, output)) {
    if (error) {
      *error = output.error_message;
    }
    return false;
  }

  std::vector<std::string> deps = output.dependencies;
  // Normalize to absolute paths for stable hashing across cwd changes.
  for (auto& dep : deps) {
    std::error_code ec;
    auto abs = std::filesystem::absolute(dep, ec);
    if (!ec) {
      dep = abs.lexically_normal().string();
    }
  }
  // Ensure primary source is present.
  {
    std::error_code ec;
    const std::string source =
        std::filesystem::absolute(unit.source_path, ec).lexically_normal().string();
    if (std::ranges::find(deps, source) == deps.end()) {
      deps.push_back(source);
    }
  }

  MetaFile meta{
      .content_hash =
          compute_content_hash(unit, deps, output.dxc_version, output.metal_ir_converter_version),
      .entry = unit.entry,
      .stage = stage_name(unit.stage),
      .profile = profile_for_stage(unit.stage),
      .defines = unit.defines,
      .dependencies = deps,
      .dxc_version = output.dxc_version,
      .metal_ir_converter_version = output.metal_ir_converter_version,
  };

  std::string write_error;
  if (!core::write_bytes(metallib_path(roots, unit), output.metallib.data(), output.metallib.size(),
                         write_error) ||
      !core::write_bytes(dxil_path(roots, unit), output.dxil.data(), output.dxil.size(),
                         write_error) ||
      !save_meta(meta_path(roots, unit), meta, write_error)) {
    if (error) {
      *error = write_error;
    }
    return false;
  }

  LINFO("compiled shader {} / {} → {}", unit.technique_name, unit.entry,
        metallib_path(roots, unit).string());
  return true;
}

}  // namespace

ShaderCache::ShaderCache(ShaderCache::CacheRoots roots) : roots_(std::move(roots)) {}

ShaderCache::EnsureStats ShaderCache::ensure_all(bool force) {
  EnsureStats stats;
  for (const ShaderTechniqueDesc& tech : ShaderTechniqueRegistry::all()) {
    EnsureStats one = ensure_technique(tech.name, force);
    stats.compiled += one.compiled;
    stats.up_to_date += one.up_to_date;
    stats.failed += one.failed;
    if (stats.first_error.empty()) {
      stats.first_error = std::move(one.first_error);
    }
  }
  return stats;
}

ShaderCache::EnsureStats ShaderCache::ensure_technique(std::string_view name, bool force) {
  EnsureStats stats;
  const ShaderTechniqueDesc* tech = ShaderTechniqueRegistry::find(name);
  if (!tech) {
    ++stats.failed;
    stats.first_error = "Unknown technique: " + std::string(name);
    LERROR("{}", stats.first_error);
    return stats;
  }

  for (const ShaderTechniqueDesc::StageDesc& stage : tech->stages) {
    CacheUnit unit;
    std::string error;
    if (!make_unit(roots_, *tech, stage, unit, &error)) {
      ++stats.failed;
      if (stats.first_error.empty()) {
        stats.first_error = error;
      }
      LERROR("{}", error);
      continue;
    }

    std::string reason;
    const bool outdated = force || unit_is_outdated(roots_, unit, &reason);
    if (!outdated) {
      ++stats.up_to_date;
      continue;
    }

    if (!ensure_unit(roots_, unit, /*force=*/true, &error)) {
      ++stats.failed;
      if (stats.first_error.empty()) {
        stats.first_error = error;
      }
      LERROR("Failed to compile {} / {}: {}", unit.technique_name, unit.entry, error);
      continue;
    }
    ++stats.compiled;
  }
  return stats;
}

bool ShaderCache::is_outdated(std::string_view technique, rhi::ShaderType stage,
                              std::string* reason) const {
  CacheUnit unit;
  std::string error;
  if (!make_unit(roots_, technique, stage, unit, &error)) {
    if (reason) {
      *reason = error;
    }
    return true;
  }
  return unit_is_outdated(roots_, unit, reason);
}

void ShaderCache::register_loaded(std::string_view technique, rhi::ShaderType stage) {
  std::scoped_lock lock(registered_mu_);
  registered_shaders_.emplace(std::string(technique), stage);
}

size_t ShaderCache::registered_shader_count() const {
  std::scoped_lock lock(registered_mu_);
  return registered_shaders_.size();
}

bool ShaderCache::any_registered_outdated() const {
  std::scoped_lock lock(registered_mu_);
  for (const auto& [technique, stage] : registered_shaders_) {
    if (is_outdated(technique, stage)) {
      return true;
    }
  }
  return false;
}

bool ShaderCache::load_shader(std::string_view technique, rhi::ShaderType stage,
                              std::vector<uint8_t>& out, std::string* error) {
  CacheUnit unit;
  std::string local_error;
  std::string& err = error ? *error : local_error;
  if (!make_unit(roots_, technique, stage, unit, &err)) {
    return false;
  }
  if (!core::read_bytes(metallib_path(roots_, unit), out, err)) {
    return false;
  }
  register_loaded(technique, stage);
  return true;
}

}  // namespace gfx
