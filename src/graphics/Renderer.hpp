#pragma once

#include "graphics/rhi/Graphics.hpp"
#include "graphics/shaders/ShaderCache.hpp"
#include "graphics/util/BumpAllocator.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

namespace gfx {
namespace rhi {
class Device;
}

class Renderer {
public:
  Renderer();
  ~Renderer();
  struct InitInfo {
    SDL_Window* window{};
    std::filesystem::path shader_root;
    std::filesystem::path cache_root;
  };
  void init(const InitInfo& info);
  void render();

private:
  void reload_shaders();
  /// Load a cooked metallib by logical technique id + stage (entry from registry).
  void load_shader(rhi::Shader& shader, std::string_view technique, rhi::ShaderType stage);

  SDL_Window* window_{};
  std::optional<ShaderCache> shader_cache_;
  std::unique_ptr<gfx::rhi::Device> device_;
  gfx::rhi::Swapchain swapchain_;

  rhi::Shader basic_vs_;
  rhi::Shader basic_fs_;
  rhi::Shader basic_cs_;
  rhi::Pipeline basic_;
  rhi::Pipeline post_process_;
  rhi::Pipeline basic_comp_pso_;
  size_t frame_num_{};
  size_t frames_in_flight_{2};

  rhi::Texture color_att_tex_;
  rhi::Texture depth_att_tex_;
  rhi::Buffer vert_buf_;
  rhi::Buffer index_buf_;
  rhi::Buffer frame_scratch_buffers_[2];
  std::optional<BumpAllocator> frame_allocators_[2];
  rhi::Buffer test_gpu_allocation_buffer_;
  rhi::CmdPool cmd_pools_[rhi::k_max_frames_in_flight];
};
} // namespace gfx
