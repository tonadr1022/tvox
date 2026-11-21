#include "graphics/shaders/ShaderCache.hpp"

#include "core/FileIo.hpp"
#include "core/Logger.hpp"
#include "graphics/shaders/ShaderCompiler.hpp"
#include "graphics/shaders/TechniqueRegistry.hpp"

#include <algorithm>
#include <expected>
#include <nlohmann/json.hpp>

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

struct MetaFile {
  std::string content_hash;
  std::string entry;
  std::string stage;
  std::string profile;
  std::vector<std::string> defines;
  std::vector<std::string> dependencies;
  std::string slang_version;
};

std::expected<MetaFile, std::string> load_meta(const std::filesystem::path& path) {
  MetaFile meta;
  auto bytes = core::read_binary_file(path);
  if (!bytes) {
    return std::unexpected{bytes.error()};
  }

  try {
    const json j = json::parse(bytes.value().begin(), bytes.value().end());
    meta.content_hash = j.at("content_hash").get<std::string>();
    meta.entry = j.at("entry").get<std::string>();
    meta.stage = j.at("stage").get<std::string>();
    meta.profile = j.at("profile").get<std::string>();
    meta.slang_version = j.at("slang_version").get<std::string>();
    meta.defines = j.value("defines", std::vector<std::string>{});
    meta.dependencies = j.value("dependencies", std::vector<std::string>{});
  } catch (const json::exception& ex) {
    return std::unexpected{std::format("Malformed shader meta: {} {}", path.string(), ex.what())};
  }
  return meta;
}

std::expected<void, std::string> save_meta(const std::filesystem::path& path,
                                           const MetaFile& meta) {
  const json j = {
      {"content_hash", meta.content_hash},
      {"entry", meta.entry},
      {"stage", meta.stage},
      {"profile", meta.profile},
      {"defines", meta.defines},
      {"dependencies", meta.dependencies},
      {"slang_version", meta.slang_version},
  };
  const std::string text = j.dump(2) + '\n';
  return core::write_text_file(path, text);
}

std::vector<std::string> sorted_copy(std::vector<std::string> values) {
  std::ranges::sort(values);
  return values;
}

std::string compute_content_hash(const CacheUnit& unit,
                                 const std::vector<std::string>& dependencies,
                                 std::string_view slang_version) {
  uint64_t hash = k_fnv_offset;
  fnv1a_update(hash, profile_for_stage(unit.stage));
  fnv1a_update(hash, unit.entry);
  fnv1a_update(hash, stage_name(unit.stage));
  for (const auto& define : sorted_copy(unit.defines)) {
    fnv1a_update(hash, define);
  }
  fnv1a_update(hash, slang_version);

  auto deps = sorted_copy(dependencies);
  for (const auto& dep : deps) {
    fnv1a_update(hash, dep);
    const auto bytes = core::read_binary_file(dep);
    if (!bytes) {
      // Missing dep → unstable hash, forces rebuild next time if it reappears.
      fnv1a_update(hash, std::string_view{"<missing>"});
      continue;
    }
    fnv1a_update(hash, bytes->data(), bytes->size());
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

std::expected<CacheUnit, std::string> make_unit(const ShaderCache::CacheRoots& roots,
                                                const ShaderTechniqueDesc& tech,
                                                const ShaderTechniqueDesc::StageDesc& stage) {
  CacheUnit out{};
  if (roots.shader_root.empty()) {
    return std::unexpected{"Shader root empty"};
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
  return out;
}

std::expected<CacheUnit, std::string> make_unit(const ShaderCache::CacheRoots& roots,
                                                std::string_view technique_name,
                                                rhi::ShaderType stage) {
  const ShaderTechniqueDesc* tech = ShaderTechniqueRegistry::find(technique_name);
  if (!tech) {
    return std::unexpected{std::format("Unknown technique: {}", technique_name)};
  }

  for (const ShaderTechniqueDesc::StageDesc& stage_desc : tech->stages) {
    if (stage_desc.stage == stage) {
      return make_unit(roots, *tech, stage_desc);
    }
  }

  return std::unexpected{
      std::format("Technique '{}' has no stage {}", technique_name, stage_name(stage)),
  };
}

std::optional<std::string> unit_is_outdated(const ShaderCache::CacheRoots& roots,
                                            const CacheUnit& unit) {
  const auto metallib = metallib_path(roots, unit);
  const auto meta_file = meta_path(roots, unit);
  std::error_code ec;
  if (!std::filesystem::is_regular_file(metallib, ec)) {
    return "metallib file doesn't exist: " + metallib.string();
  }

  // Packaged builds may omit meta/ → treat existing metallib as prebuilt (up to date).
  if (!std::filesystem::is_regular_file(meta_file, ec)) {
    return "meta file doesn't exist: " + meta_file.string();
  }

  auto meta_result = load_meta(meta_file);
  if (!meta_result) {
    return "failed to load meta: " + meta_result.error();
  }
  auto& parsed_meta = meta_result.value();

  if (parsed_meta.entry != unit.entry || parsed_meta.stage != stage_name(unit.stage) ||
      parsed_meta.profile != profile_for_stage(unit.stage) ||
      sorted_copy(parsed_meta.defines) != sorted_copy(unit.defines)) {
    return "entrypoint/stage/profile doesn't match";
  }

  // TODO: fix this api
  const ShaderToolVersions tools = query_shader_tool_versions();
  if (!tools.ok()) {
    return "tools version failed to load: " + tools.error_message;
  }
  if (tools.slang_version != parsed_meta.slang_version) {
    return std::format("slang version mismatch: current: {}, meta: {}", tools.slang_version,
                       parsed_meta.slang_version);
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
    return "meta missing source dependency";
  }

  const std::string hash =
      compute_content_hash(unit, parsed_meta.dependencies, tools.slang_version);
  if (hash != parsed_meta.content_hash) {
    return "content hash mismatch";
  }

  return std::nullopt;
}

} // namespace

std::expected<void, std::string> ShaderCache::ensure_unit(const ShaderCache::CacheRoots& roots,
                                                          const CacheUnit& unit, bool force) {
  if (!force) {
    const auto outdated_reason = unit_is_outdated(roots, unit);
    if (!outdated_reason.has_value()) {
      return {};
    }
    LINFO("shader outdated ({}): {} / {}", outdated_reason.value(), unit.technique_name,
          unit.entry);
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

  const auto shader_compile_output = shader_compiler_.compile(input);
  if (!shader_compile_output) {
    return std::unexpected{shader_compile_output.error()};
  }
  LINFO("compiled shader {} / {} → {}", unit.technique_name, unit.entry,
        metallib_path(roots, unit).string());

  std::vector<std::string> dependency_paths = shader_compile_output->dependency_paths;
  // Normalize to absolute paths for stable hashing across cwd changes.
  for (auto& dependency_path : dependency_paths) {
    std::error_code ec;
    auto abs_path = std::filesystem::absolute(dependency_path, ec);
    if (!ec) {
      dependency_path = abs_path.lexically_normal().string();
    }
  }

  // Ensure primary source is present.
  {
    std::error_code ec;
    const std::string source =
        std::filesystem::absolute(unit.source_path, ec).lexically_normal().string();
    if (std::ranges::find(dependency_paths, source) == dependency_paths.end()) {
      dependency_paths.push_back(source);
    }
  }

  MetaFile meta{
      .content_hash =
          compute_content_hash(unit, dependency_paths, shader_compile_output->slang_version),
      .entry = unit.entry,
      .stage = stage_name(unit.stage),
      .profile = profile_for_stage(unit.stage),
      .defines = unit.defines,
      .dependencies = dependency_paths,
      .slang_version = shader_compile_output->slang_version,
  };

  return core::write_binary_file(metallib_path(roots, unit), shader_compile_output->bytes)
      .and_then([&] {
        return core::write_binary_file(dxil_path(roots, unit), shader_compile_output->bytes);
      })
      .and_then([&] { return save_meta(meta_path(roots, unit), meta); });
}

ShaderCache::ShaderCache(ShaderCache::CacheRoots roots) : roots_(std::move(roots)) {
  // TODO: explicit init shader cache? or raii everything?
  shader_compiler_.init();
}

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
    auto unit = make_unit(roots_, *tech, stage);
    if (!unit) {
      stats.failed++;
      LERROR("{}", unit.error());

      // TODO: maybe don't do this?
      if (stats.first_error.empty()) {
        stats.first_error = unit.error();
      }

      continue;
    }

    const bool outdated = force || unit_is_outdated(roots_, unit.value());
    if (!outdated) {
      ++stats.up_to_date;
      continue;
    }

    const auto unit_ensured = ensure_unit(roots_, unit.value(), false);
    if (!unit_ensured) {
      ++stats.failed;
      if (stats.first_error.empty()) {
        stats.first_error = unit_ensured.error();
      }
      LERROR("Failed to compile {} / {}: {}", unit->technique_name, unit->entry,
             unit_ensured.error());
      continue;
    }
    ++stats.compiled;
  }
  return stats;
}

std::optional<std::string> ShaderCache::is_outdated(std::string_view technique,
                                                    rhi::ShaderType stage) const {
  // TODO: an error here might mean a real error?
  const auto unit = make_unit(roots_, technique, stage);
  if (!unit) {
    return unit.error();
  }
  return unit_is_outdated(roots_, unit.value());
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
  return std::ranges::any_of(registered_shaders_,
                             [this](const std::pair<std::string, rhi::ShaderType>& pair) {
                               return is_outdated(pair.first, pair.second).has_value();
                             });
}

std::expected<std::vector<std::byte>, std::string>
ShaderCache::load_shader(std::string_view technique, rhi::ShaderType stage) {
  const auto unit = make_unit(roots_, technique, stage);
  if (!unit) {
    return std::unexpected{unit.error()};
  }
  const auto bytes = core::read_binary_file(metallib_path(roots_, unit.value()));
  if (!bytes.has_value()) {
    return std::unexpected{bytes.error()};
  }

  register_loaded(technique, stage);

  return bytes.value();
}

} // namespace gfx
