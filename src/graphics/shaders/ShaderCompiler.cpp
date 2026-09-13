#include "graphics/shaders/ShaderCompiler.hpp"

#include <clocale>
#include <cstring>
#include <mutex>
#include <string_view>

#include "core/DynamicLib.hpp"
#include "core/FileIo.hpp"
#include "core/Paths.hpp"
#include "graphics/shaders/MetalIrConverter.hpp"

#define ComPtr CComPtr
#include <dxcapi.h>

namespace gfx {
namespace {

using DxcCreateInstanceFn = HRESULT (*)(REFCLSID, REFIID, LPVOID*);

std::mutex g_search_paths_mu;
std::vector<std::filesystem::path> g_extra_search_paths;

std::wstring utf8_to_wide(std::string_view utf8) {
  std::wstring out;
  out.reserve(utf8.size());
  for (unsigned char c : utf8) {
    out.push_back(static_cast<wchar_t>(c));
  }
  return out;
}

std::string wide_to_utf8(const wchar_t* wide) {
  if (!wide) {
    return {};
  }
  std::string out;
  for (const wchar_t* p = wide; *p; ++p) {
    out.push_back(static_cast<char>(*p));
  }
  return out;
}

std::vector<std::filesystem::path> library_search_paths() {
  std::vector<std::filesystem::path> paths;
  {
    std::scoped_lock lock(g_search_paths_mu);
    paths = g_extra_search_paths;
  }
  if (auto exe_dir = core::executable_directory(); !exe_dir.empty()) {
    paths.push_back(std::move(exe_dir));
  }
#ifdef TVOX_SHADER_LIBS_BIN
  paths.emplace_back(TVOX_SHADER_LIBS_BIN);
#endif
  paths.emplace_back(std::filesystem::current_path());
  return paths;
}

const char* profile_for_stage(ShaderStage stage) {
  switch (stage) {
    case ShaderStage::Vertex:
      return "vs_6_6";
    case ShaderStage::Fragment:
      return "ps_6_6";
    case ShaderStage::Compute:
      return "cs_6_6";
    case ShaderStage::Mesh:
      return "ms_6_6";
    case ShaderStage::Task:
      return "as_6_6";
  }
  return "vs_6_6";
}

struct DxcState {
  core::DynamicLib lib;
  DxcCreateInstanceFn create_instance{nullptr};
  std::string version;
  std::string load_error;

  DxcState() {
    const auto dirs = library_search_paths();
    const std::string filename = core::shared_library_filename("dxcompiler");
    if (!lib.open_search(filename, dirs, load_error)) {
      load_error +=
          "Place libdxcompiler next to the executable or in third_party/shader_libs/bin/.";
      return;
    }
    create_instance = lib.symbol<DxcCreateInstanceFn>("DxcCreateInstance", load_error);
    if (!create_instance) {
      return;
    }

    ComPtr<IDxcCompiler3> compiler;
    // NOLINTNEXTLINE
    if (FAILED(create_instance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler))) || !compiler) {
      load_error = "DxcCreateInstance(CLSID_DxcCompiler) failed";
      create_instance = nullptr;
      return;
    }
    ComPtr<IDxcVersionInfo> info;
    // NOLINTNEXTLINE
    if (SUCCEEDED(compiler->QueryInterface(IID_PPV_ARGS(&info))) && info) {
      UINT32 major = 0;
      UINT32 minor = 0;
      info->GetVersion(&major, &minor);
      version = std::to_string(major) + "." + std::to_string(minor);
    } else {
      version = "unknown";
    }
  }
};

DxcState& dxc_state() {
  static DxcState state;
  return state;
}

bool compile_dxil(const ShaderCompileInput& input, ShaderCompileOutput& out,
                  std::vector<uint8_t>& dxil) {
  DxcState& dxc = dxc_state();
  if (!dxc.create_instance) {
    out.error_message = dxc.load_error.empty() ? "DXC is not available" : dxc.load_error;
    return false;
  }
  out.dxc_version = dxc.version;

  std::vector<uint8_t> source;
  if (!core::read_bytes(input.source_path, source, out.error_message)) {
    return false;
  }

  ComPtr<IDxcUtils> utils;
  ComPtr<IDxcCompiler3> compiler;
  // NOLINTNEXTLINE
  if (FAILED(dxc.create_instance(CLSID_DxcUtils, IID_PPV_ARGS(&utils))) || !utils) {
    out.error_message = "DxcCreateInstance(CLSID_DxcUtils) failed";
    return false;
  }
  // NOLINTNEXTLINE
  if (FAILED(dxc.create_instance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler))) || !compiler) {
    out.error_message = "DxcCreateInstance(CLSID_DxcCompiler) failed";
    return false;
  }

  std::vector<std::wstring> args_storage;
  args_storage.emplace_back(L"-Wno-conversion");
  args_storage.emplace_back(L"-D");
  args_storage.emplace_back(L"__metal__");
  if (input.disable_optimization) {
    args_storage.emplace_back(L"-Od");
  }
  if (input.embed_debug) {
    args_storage.emplace_back(L"-Zi");
    args_storage.emplace_back(L"-Qembed_debug");
  }
  args_storage.emplace_back(L"-T");
  args_storage.emplace_back(utf8_to_wide(profile_for_stage(input.stage)));
  args_storage.emplace_back(L"-E");
  args_storage.emplace_back(utf8_to_wide(input.entry_point));

  for (const auto& define : input.defines) {
    args_storage.emplace_back(L"-D");
    args_storage.emplace_back(utf8_to_wide(define));
  }
  for (const auto& include_dir : input.include_directories) {
    args_storage.emplace_back(L"-I");
    args_storage.emplace_back(utf8_to_wide(include_dir));
  }
  args_storage.emplace_back(utf8_to_wide(input.source_path.filename().string()));

  std::vector<LPCWSTR> args;
  args.reserve(args_storage.size());
  for (const auto& arg : args_storage) {
    args.push_back(arg.c_str());
  }

  DxcBuffer source_buffer{
      .Ptr = source.data(),
      .Size = source.size(),
      .Encoding = DXC_CP_UTF8,
  };

  struct IncludeHandler final : IDxcIncludeHandler {
    ComPtr<IDxcIncludeHandler> default_handler;
    std::vector<std::string>* dependencies{nullptr};

    HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR filename, IDxcBlob** include_source) override {
      const HRESULT hr = default_handler->LoadSource(filename, include_source);
      if (SUCCEEDED(hr) && dependencies) {
        dependencies->push_back(wide_to_utf8(filename));
      }
      return hr;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
      return default_handler->QueryInterface(riid, object);
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return 0; }
    ULONG STDMETHODCALLTYPE Release() override { return 0; }
  } include_handler;

  include_handler.dependencies = &out.dependencies;
  if (FAILED(utils->CreateDefaultIncludeHandler(&include_handler.default_handler))) {
    out.error_message = "CreateDefaultIncludeHandler failed";
    return false;
  }

  out.dependencies.push_back(input.source_path.string());

  // Workaround: https://github.com/microsoft/DirectXShaderCompiler/issues/7869
  static std::mutex locale_mu;
  static char* prev_locale = nullptr;
  static int locale_scope = 0;
  {
    std::scoped_lock lock(locale_mu);
    if (locale_scope++ == 0) {
      prev_locale = strdup(setlocale(LC_ALL, nullptr));
      setlocale(LC_ALL, "en_US.UTF-8");
    }
  }

  ComPtr<IDxcResult> result;
  const HRESULT compile_hr =
      compiler->Compile(&source_buffer, args.data(), static_cast<UINT32>(args.size()),
                        // NOLINTNEXTLINE
                        &include_handler, IID_PPV_ARGS(&result));

  {
    std::scoped_lock lock(locale_mu);
    if (--locale_scope == 0) {
      setlocale(LC_ALL, prev_locale);
      free(prev_locale);
      prev_locale = nullptr;
    }
  }

  if (FAILED(compile_hr) || !result) {
    out.error_message = "IDxcCompiler3::Compile failed";
    return false;
  }

  ComPtr<IDxcBlobUtf8> errors;
  // NOLINTNEXTLINE
  result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
  if (errors && errors->GetStringLength() > 0) {
    out.error_message.assign(errors->GetStringPointer(), errors->GetStringLength());
  }

  // NOLINTNEXTLINE
  HRESULT status = E_FAIL;
  result->GetStatus(&status);
  if (FAILED(status)) {
    if (out.error_message.empty()) {
      out.error_message = "DXC compilation failed";
    }
    return false;
  }

  ComPtr<IDxcBlob> shader_blob;
  // NOLINTNEXTLINE
  result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader_blob), nullptr);
  if (!shader_blob || shader_blob->GetBufferSize() == 0) {
    out.error_message = "DXC produced an empty shader blob";
    return false;
  }

  const auto* bytes = static_cast<const uint8_t*>(shader_blob->GetBufferPointer());
  dxil.assign(bytes, bytes + shader_blob->GetBufferSize());
  out.error_message.clear();
  return true;
}

}  // namespace

void set_library_search_paths(std::vector<std::filesystem::path> paths) {
  std::scoped_lock lock(g_search_paths_mu);
  g_extra_search_paths = std::move(paths);
}

ShaderToolVersions query_shader_tool_versions() {
  ShaderToolVersions versions;
  DxcState& dxc = dxc_state();
  if (!dxc.create_instance) {
    versions.error_message = dxc.load_error.empty() ? "DXC is not available" : dxc.load_error;
    return versions;
  }
  versions.dxc = dxc.version;

  if (!query_metal_ir_version(library_search_paths(), versions.metal_ir_converter,
                              versions.error_message)) {
    return versions;
  }
  return versions;
}

bool compile(const ShaderCompileInput& input, ShaderCompileOutput& out) {
  out = ShaderCompileOutput{};
  if (input.source_path.empty()) {
    out.error_message = "CompileInput.source_path is empty";
    return false;
  }
  if (input.entry_point.empty()) {
    out.error_message = "CompileInput.entry_point is empty";
    return false;
  }

  std::vector<uint8_t> dxil;
  if (!compile_dxil(input, out, dxil)) {
    return false;
  }
  out.dxil = dxil;

  return convert_dxil_to_metallib(input, dxil, library_search_paths(), out);
}

}  // namespace gfx
