#include "graphics/shaders/MetalIrConverter.hpp"

#include <metal_irconverter/metal_irconverter.h>

#include <mutex>
#include <string>

#include "core/DynamicLib.hpp"

namespace gfx {
namespace {

IRShaderStage ir_stage_for(ShaderStage stage) {
  switch (stage) {
    case ShaderStage::Vertex:
      return IRShaderStageVertex;
    case ShaderStage::Fragment:
      return IRShaderStageFragment;
    case ShaderStage::Compute:
      return IRShaderStageCompute;
    case ShaderStage::Mesh:
      return IRShaderStageMesh;
    case ShaderStage::Task:
      return IRShaderStageAmplification;
  }
  return IRShaderStageVertex;
}

struct MetalIrSymbols {
  core::DynamicLib lib;
  std::string load_error;
  std::string version;

#define TVOX_IR_FN(name) decltype (&(name))(name){nullptr};
  TVOX_IR_FN(IRCompilerCreate)
  TVOX_IR_FN(IRCompilerDestroy)
  TVOX_IR_FN(IRCompilerSetMinimumGPUFamily)
  TVOX_IR_FN(IRCompilerSetCompatibilityFlags)
  TVOX_IR_FN(IRCompilerSetValidationFlags)
  TVOX_IR_FN(IRCompilerSetEntryPointName)
  TVOX_IR_FN(IRCompilerSetStageInGenerationMode)
  TVOX_IR_FN(IRCompilerSetGlobalRootSignature)
  TVOX_IR_FN(IRObjectCreateFromDXIL)
  TVOX_IR_FN(IRCompilerAllocCompileAndLink)
  TVOX_IR_FN(IRErrorDestroy)
  TVOX_IR_FN(IRErrorGetCode)
  TVOX_IR_FN(IRMetalLibBinaryCreate)
  TVOX_IR_FN(IRObjectGetMetalLibBinary)
  TVOX_IR_FN(IRMetalLibGetBytecodeSize)
  TVOX_IR_FN(IRMetalLibGetBytecode)
  TVOX_IR_FN(IRMetalLibBinaryDestroy)
  TVOX_IR_FN(IRObjectDestroy)
  TVOX_IR_FN(IRRootSignatureCreateFromDescriptor)
#undef TVOX_IR_FN

  bool load(const std::vector<std::filesystem::path>& dirs) {
    const std::string filename = core::shared_library_filename("metalirconverter");
    if (!lib.open_search(filename, dirs, load_error)) {
      load_error +=
          "Place libmetalirconverter next to the executable or in "
          "third_party/shader_libs/bin/.";
      return false;
    }

#define TVOX_IR_LOAD(name)                                  \
  do {                                                      \
    (name) = lib.symbol<decltype(name)>(#name, load_error); \
    if (!(name)) {                                          \
      return false;                                         \
    }                                                       \
  } while (0)
    TVOX_IR_LOAD(IRCompilerCreate);
    TVOX_IR_LOAD(IRCompilerDestroy);
    TVOX_IR_LOAD(IRCompilerSetMinimumGPUFamily);
    TVOX_IR_LOAD(IRCompilerSetCompatibilityFlags);
    TVOX_IR_LOAD(IRCompilerSetValidationFlags);
    TVOX_IR_LOAD(IRCompilerSetEntryPointName);
    TVOX_IR_LOAD(IRCompilerSetStageInGenerationMode);
    TVOX_IR_LOAD(IRCompilerSetGlobalRootSignature);
    TVOX_IR_LOAD(IRObjectCreateFromDXIL);
    TVOX_IR_LOAD(IRCompilerAllocCompileAndLink);
    TVOX_IR_LOAD(IRErrorDestroy);
    TVOX_IR_LOAD(IRErrorGetCode);
    TVOX_IR_LOAD(IRMetalLibBinaryCreate);
    TVOX_IR_LOAD(IRObjectGetMetalLibBinary);
    TVOX_IR_LOAD(IRMetalLibGetBytecodeSize);
    TVOX_IR_LOAD(IRMetalLibGetBytecode);
    TVOX_IR_LOAD(IRMetalLibBinaryDestroy);
    TVOX_IR_LOAD(IRObjectDestroy);
    TVOX_IR_LOAD(IRRootSignatureCreateFromDescriptor);
#undef TVOX_IR_LOAD

    version = std::to_string(IR_VERSION_MAJOR) + "." + std::to_string(IR_VERSION_MINOR) + "." +
              std::to_string(IR_VERSION_PATCH);
    load_error.clear();
    return true;
  }

  [[nodiscard]] bool ok() const { return static_cast<bool>(lib) && load_error.empty(); }
};

MetalIrSymbols& metal_ir(const std::vector<std::filesystem::path>& dirs) {
  static MetalIrSymbols symbols;
  static std::once_flag once;
  std::call_once(once, [&] { symbols.load(dirs); });
  return symbols;
}

/// Minimal root signature for the current empty RHI. Keep in lockstep with future
/// Metal binding layout (WickedEngine lesson).
IRRootSignature* default_root_signature(MetalIrSymbols& ir, std::string& error) {
  static IRRootSignature* root_sig = nullptr;
  static std::once_flag once;
  static std::string create_error;

  std::call_once(once, [&] {
    IRVersionedRootSignatureDescriptor desc{};
    desc.version = IRRootSignatureVersion_1_1;
    desc.desc_1_1.NumParameters = 0;
    desc.desc_1_1.pParameters = nullptr;
    desc.desc_1_1.NumStaticSamplers = 0;
    desc.desc_1_1.pStaticSamplers = nullptr;
    desc.desc_1_1.Flags = IRRootSignatureFlagAllowInputAssemblerInputLayout;

    IRError* root_error = nullptr;
    root_sig = ir.IRRootSignatureCreateFromDescriptor(&desc, &root_error);
    if (!root_sig) {
      const uint32_t code = root_error ? ir.IRErrorGetCode(root_error) : 0;
      create_error =
          "IRRootSignatureCreateFromDescriptor failed (code " + std::to_string(code) + ")";
      if (root_error) {
        ir.IRErrorDestroy(root_error);
      }
    }
  });

  if (!root_sig) {
    error = create_error;
  }
  return root_sig;
}

}  // namespace

bool convert_dxil_to_metallib(const ShaderCompileInput& input, const std::vector<uint8_t>& dxil,
                              const std::vector<std::filesystem::path>& library_dirs,
                              ShaderCompileOutput& out) {
  MetalIrSymbols& ir = metal_ir(library_dirs);
  if (!ir.ok()) {
    out.error_message =
        ir.load_error.empty() ? "Metal IR Converter is not available" : ir.load_error;
    return false;
  }
  out.metal_ir_converter_version = ir.version;

  IRRootSignature* root_sig = default_root_signature(ir, out.error_message);
  if (!root_sig) {
    return false;
  }

  IRCompiler* compiler = ir.IRCompilerCreate();
  if (!compiler) {
    out.error_message = "IRCompilerCreate failed";
    return false;
  }

  ir.IRCompilerSetMinimumGPUFamily(compiler, IRGPUFamilyMetal3);
  ir.IRCompilerSetCompatibilityFlags(
      compiler, IRCompatibilityFlags(
                    IRCompatibilityFlagBoundsCheck | IRCompatibilityFlagPositionInvariance |
                    IRCompatibilityFlagTextureMinLODClamp | IRCompatibilityFlagSamplerLODBias));
  ir.IRCompilerSetValidationFlags(compiler, IRCompilerValidationFlagAll);
  ir.IRCompilerSetEntryPointName(compiler, input.entry_point.c_str());
  ir.IRCompilerSetGlobalRootSignature(compiler, root_sig);
  ir.IRCompilerSetStageInGenerationMode(compiler, IRStageInCodeGenerationModeUseMetalVertexFetch);

  IRObject* dxil_object =
      ir.IRObjectCreateFromDXIL(dxil.data(), dxil.size(), IRBytecodeOwnershipNone);
  IRError* convert_error = nullptr;
  IRObject* metal_object =
      ir.IRCompilerAllocCompileAndLink(compiler, nullptr, dxil_object, &convert_error);
  if (!metal_object) {
    const uint32_t code = convert_error ? ir.IRErrorGetCode(convert_error) : 0;
    out.error_message = "IRCompilerAllocCompileAndLink failed (code " + std::to_string(code) + ")";
    if (convert_error) {
      ir.IRErrorDestroy(convert_error);
    }
    ir.IRObjectDestroy(dxil_object);
    ir.IRCompilerDestroy(compiler);
    return false;
  }

  IRMetalLibBinary* metallib = ir.IRMetalLibBinaryCreate();
  ir.IRObjectGetMetalLibBinary(metal_object, ir_stage_for(input.stage), metallib);
  const size_t size = ir.IRMetalLibGetBytecodeSize(metallib);
  out.metallib.resize(size);
  if (size > 0) {
    ir.IRMetalLibGetBytecode(metallib, out.metallib.data());
  }

  ir.IRMetalLibBinaryDestroy(metallib);
  ir.IRObjectDestroy(dxil_object);
  ir.IRObjectDestroy(metal_object);
  ir.IRCompilerDestroy(compiler);

  if (out.metallib.empty()) {
    out.error_message = "Metal IR Converter produced an empty metallib";
    return false;
  }
  return true;
}

bool query_metal_ir_version(const std::vector<std::filesystem::path>& library_dirs,
                            std::string& version, std::string& error) {
  MetalIrSymbols& ir = metal_ir(library_dirs);
  if (!ir.ok()) {
    error = ir.load_error.empty() ? "Metal IR Converter is not available" : ir.load_error;
    return false;
  }
  version = ir.version;
  error.clear();
  return true;
}

}  // namespace gfx
