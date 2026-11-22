#include "graphics/shaders/ShaderCompiler.hpp"

#include "core/Logger.hpp"
#include "core/Util.hpp"
#include "util/StopWatch.hpp"

#include <cstring>
#include <expected>

#include "graphics/rhi/ShaderType.hpp"
#include "slang.h"

using namespace slang;

namespace gfx {

namespace {

void log_if_error(Slang::ComPtr<IBlob>& diagnostics) {
  if (diagnostics) {
    LERROR("{}", std::string(reinterpret_cast<const char*>(diagnostics->getBufferPointer()),
                             diagnostics->getBufferSize()));
  }
}

SlangStage convert_slang_stage(rhi::ShaderType type) {
  switch (type) {
  case gfx::rhi::ShaderType::Compute:
    return SLANG_STAGE_COMPUTE;
  case gfx::rhi::ShaderType::Fragment:
    return SLANG_STAGE_FRAGMENT;
  case gfx::rhi::ShaderType::Vertex:
    return SLANG_STAGE_VERTEX;
  case gfx::rhi::ShaderType::Mesh:
    return SLANG_STAGE_MESH;
  case gfx::rhi::ShaderType::Task:
    return SLANG_STAGE_AMPLIFICATION;
  case gfx::rhi::ShaderType::None:
    break;
  }
  FATAL(false, "Unsupported stage: {}", (int)type);
}

} // namespace

ShaderToolVersions query_shader_tool_versions() {
  ShaderToolVersions versions;

  // Bump this when shader compiler options change so cached metallibs are rebuilt.
  versions.slang_version = "v2-metal-debug-info";
  versions.error_message = "";

  return versions;
}

void ShaderCompiler::init_if_needed() {
  if (initialized_global_session_) {
    return;
  }
  initialized_global_session_ = true;

  Stopwatch sw;
  SlangGlobalSessionDesc desc{};
  createGlobalSession(&desc, global_session_.writeRef());

  TargetDesc target_desc{};
  target_desc.format = SLANG_METAL_LIB;

  const char* search_paths[] = {"/Users/tony/personal/tvox/resources/shaders/passes"};
  const CompilerOptionEntry compiler_options[] = {
      CompilerOptionEntry{
          .name = CompilerOptionName::DebugInformation,
          .value = {.kind = CompilerOptionValueKind::Int,
                    .intValue0 = SLANG_DEBUG_INFO_LEVEL_MINIMAL},
      },
      CompilerOptionEntry{
          .name = CompilerOptionName::DownstreamArgs,
          .value = {.kind = CompilerOptionValueKind::String,
                    .stringValue0 = "metal",
                    .stringValue1 = "-frecord-sources\n-gline-tables-only"},
      },
  };

  SessionDesc session_desc{
      .targets = &target_desc,
      .targetCount = 1,
      .searchPaths = search_paths,
      .searchPathCount = ARRAY_SIZE(search_paths),
      .compilerOptionEntries = compiler_options,
      .compilerOptionEntryCount = ARRAY_SIZE(compiler_options),
  };
  global_session_->createSession(session_desc, session_.writeRef());
  LINFO("initialized shader compiler in {} ms", sw.elapsed_milliseconds());
}

std::expected<ShaderCompileOutput, std::string>
ShaderCompiler::compile(const ShaderCompileInput& compile_input) {
  init_if_needed();
  if (compile_input.source_path.empty()) {
    return std::unexpected("CompileInput.source_path is empty");
  }
  if (compile_input.entry_point.empty()) {
    return std::unexpected("CompileInput.entry_point is empty");
  }

  auto slang_path = compile_input.source_path;
  slang_path.replace_extension(".slang");
  if (!std::filesystem::is_regular_file(slang_path)) {
    return std::unexpected{
        std::format("CompileInput.source_path slang file does not exist: {}", slang_path.string())};
  }

  Slang::ComPtr<IBlob> diagnostics{};
  Slang::ComPtr<IModule> module{
      session_->loadModule(compile_input.source_path.string().c_str(), diagnostics.writeRef())};
  if (!module || diagnostics) {
    log_if_error(diagnostics);
    return std::unexpected("failed to load module");
  }

  Slang::ComPtr<IEntryPoint> entry_point;
  if (SLANG_FAILED(module->findAndCheckEntryPoint(
          compile_input.entry_point.c_str(), convert_slang_stage(compile_input.stage),
          entry_point.writeRef(), diagnostics.writeRef()))) {
    log_if_error(diagnostics);
    return std::unexpected(std::format("Failed to find entry point: entry_point={}, stage={}",
                                       compile_input.entry_point, to_string(compile_input.stage)));
  }

  IComponentType* components[] = {entry_point};
  Slang::ComPtr<IComponentType> program;
  session_->createCompositeComponentType(components, ARRAY_SIZE(components), program.writeRef());

  Slang::ComPtr<IComponentType> linked_program;
  program->link(linked_program.writeRef(), diagnostics.writeRef());
  log_if_error(diagnostics);

  Slang::ComPtr<IBlob> kernel_blob;
  linked_program->getEntryPointCode(0, 0, kernel_blob.writeRef(), diagnostics.writeRef());
  log_if_error(diagnostics);
  std::vector<std::byte> bytes(kernel_blob->getBufferSize());
  memcpy(bytes.data(), kernel_blob->getBufferPointer(), kernel_blob->getBufferSize());
  ASSERT(bytes.size() > 0);

  std::vector<std::string> dependency_paths;
  const auto* reflection = module->getModuleReflection();
  dependency_paths.reserve(module->getDependencyFileCount());
  for (size_t i = 0; i < module->getDependencyFileCount(); i++) {
    dependency_paths.emplace_back(module->getDependencyFilePath(i));
  }

  return ShaderCompileOutput{
      .bytes = bytes,
      .dependency_paths = dependency_paths,
      .slang_version = "v2-metal-debug-info",
  };
}

} // namespace gfx
