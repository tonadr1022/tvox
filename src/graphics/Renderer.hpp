#pragma once

#include "graphics/rhi/Graphics.hpp"
#include "graphics/shaders/ShaderCache.hpp"
#include "graphics/util/BumpAllocator.hpp"
#include "graphics/util/IndexAllocator.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>

namespace gfx {
namespace rhi {
class Device;
}

struct BindlessTexture : rhi::Texture {
  uint32_t index;
};

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
  void recreate_textures();
  /// Load a cooked metallib by logical technique id + stage (entry from registry).
  void load_shader(rhi::Shader& shader, std::string_view technique, rhi::ShaderType stage);

  SDL_Window* window_{};
  std::optional<ShaderCache> shader_cache_;
  std::unique_ptr<gfx::rhi::Device> device_;
  gfx::rhi::Swapchain swapchain_;

  rhi::TimelinePoint latest_completion_sema_;
  rhi::Shader basic_vs_;
  rhi::Shader basic_fs_;
  rhi::Shader basic_cs_;
  rhi::Shader post_process_vs_;
  rhi::Shader post_process_fs_;
  rhi::Pipeline basic_;
  rhi::Pipeline post_process_;
  rhi::Pipeline basic_comp_pso_;
  size_t frame_num_{};
  size_t frames_in_flight_{2};

  BindlessTexture color_att_tex_;
  BindlessTexture depth_att_tex_;
  rhi::Buffer vert_buf_;
  rhi::Buffer index_buf_;
  rhi::Buffer frame_scratch_buffers_[2];
  std::optional<BumpAllocator> frame_allocators_[2];
  rhi::Buffer test_gpu_allocation_buffer_;
  rhi::CmdPool cmd_pools_[rhi::k_max_frames_in_flight];
  IndexAllocator texture_descriptor_index_allocator_;
};
} // namespace gfx
