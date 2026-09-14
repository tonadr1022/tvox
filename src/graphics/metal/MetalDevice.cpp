#include "MetalDevice.hpp"

#include <Foundation/NSSharedPtr.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <string>

#include "core/EAssert.hpp"
#include "core/Logger.hpp"
#include "core/Util.hpp"
#include "graphics/metal/MetalTypes.hpp"
#include "graphics/metal/MetalWindow.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"

namespace gfx::metal {
namespace {

MTL::PixelFormat to_metal_format(rhi::Format format) {
  switch (format) {
    case rhi::Format::BGRA8_UNORM:
      return MTL::PixelFormatBGRA8Unorm;
    case rhi::Format::Unknown:
      return MTL::PixelFormatInvalid;
  }
  return MTL::PixelFormatInvalid;
}

}  // namespace

MetalDevice::~MetalDevice() = default;

void MetalDevice::init() {
  device_ = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

  frames_in_flight_ = rhi::k_max_frames_in_flight;

  queue_ = NS::TransferPtr(device_->newMTL4CommandQueue());

  for (int i = 0; i < frames_in_flight_; i++) {
    auto& frame = per_frame_[i];
    frame.cmd_allocator = NS::TransferPtr(device_->newCommandAllocator());
    frame.cmd_buf = NS::TransferPtr(device_->newCommandBuffer());
    frame.cmd_encoder.emplace();
    frame.cmd_encoder->set_device(this);
    frame.cmd_encoder->set_cmd_buffer(frame.cmd_buf);
    frame.fence = NS::TransferPtr(device_->newSharedEvent());
  }
}

void MetalDevice::create_swapchain(const rhi::SwapchainDesc& desc, SDL_Window* window,
                                   rhi::Swapchain& swapchain) {
  swapchain.desc = desc;
  swapchain.desc.format = rhi::Format::BGRA8_UNORM;

  if (!swapchain.internal_data) {
    swapchain.internal_data = wi::allocator::make_shared<Swapchain_Metal>();
    LINFO("made internal swapchain data");
  }
  auto* internal_data = to_internal(swapchain);

  if (!internal_data->layer) {
    ASSERT(window);
    internal_data->layer = NS::TransferPtr(CA::MetalLayer::layer());
    auto* layer = internal_data->layer.get();
    layer->setDevice(device_.get());
    layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    layer->setFramebufferOnly(true);
    layer->setDisplaySyncEnabled(true);
    layer->setMaximumDrawableCount(3);

    queue_->addResidencySet(layer->residencySet());
    set_layer_for_window(window, layer);
  }

  auto* layer = internal_data->layer.get();
  layer->setDrawableSize(CGSize{.width = static_cast<CGFloat>(desc.width),
                                .height = static_cast<CGFloat>(desc.height)});
}

void MetalDevice::create_pipeline(rhi::GraphicsPipelineCreateInfo&) {
  NS::SharedPtr<MTL4::RenderPipelineDescriptor> desc =
      NS::TransferPtr(MTL4::RenderPipelineDescriptor::alloc()->init());
}

bool MetalDevice::create_shader(rhi::ShaderType type, const void* data, size_t size,
                                rhi::Shader& shader, std::string_view entry_point) {
  bool success{true};
  shader.internal_data.reset();
  shader.internal_data = wi::allocator::make_shared<Shader_Metal>();
  Shader_Metal* internal_data = to_internal(shader);

  dispatch_data_t dispatch_data =
      dispatch_data_create(data, size, dispatch_get_main_queue(), nullptr);

  NS::Error* error{};
  NS::SharedPtr<MTL::Library> library = NS::TransferPtr(device_->newLibrary(dispatch_data, &error));
  if (error) {
    auto* desc = error->localizedDescription();
    LOG_ASSERT(0, "{}", desc->utf8String());
    error->release();
    success = false;
  }

  internal_data->library = library;
  const std::string entry_name = entry_point.empty() ? "main" : std::string(entry_point);
  NS::SharedPtr<NS::String> entry =
      NS::TransferPtr(NS::String::alloc()->init(entry_name.c_str(), NS::UTF8StringEncoding));

  internal_data->function = NS::TransferPtr(library->newFunction(entry.get()));
  if (!internal_data->function) {
    LOG_ASSERT(0, "Metal library has no function '{}'", entry_name);
    success = false;
  }

  // store compute pipeline directly on the shader
  if (success && type == rhi::ShaderType::Compute) {
    internal_data->compute_pipeline =
        NS::TransferPtr(device_->newComputePipelineState(internal_data->function.get(), &error));
    if (error) {
      auto* desc = error->localizedDescription();
      LOG_ASSERT(0, "{}", desc->utf8String());
      error->release();
      success = false;
    }
  }

  return success;
}

bool MetalDevice::create_pipeline(const rhi::PipelineDesc& desc, rhi::Pipeline& pipeline) {
  pipeline.desc = desc;
  pipeline.internal_data.reset();
  pipeline.internal_data = wi::allocator::make_shared<Pipeline_Metal>();
  pipelines_cache_.clear();
  return true;
}

MTL::RenderPipelineState* MetalDevice::ensure_render_pipeline(
    rhi::Pipeline& pipeline, const rhi::RenderPassInfo& renderpass_info) {
  rhi::PipelineHash hash;
  hash.pipeline = &pipeline;
  hash.renderpass_hash = renderpass_info.get_hash();

  if (auto it = pipelines_cache_.find(hash); it != pipelines_cache_.end()) {
    return it->second.get();
  }

  const rhi::PipelineDesc& desc = pipeline.desc;

  NS::SharedPtr<MTL::RenderPipelineDescriptor> render_pipeline_desc =
      NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());

  if (desc.vertex_shader) {
    auto* internal = to_internal(desc.vertex_shader);
    render_pipeline_desc->setVertexFunction(internal->function.get());
  }
  if (desc.mesh_shader) {
    auto* internal = to_internal(desc.mesh_shader);
    render_pipeline_desc->setVertexFunction(internal->function.get());
  }
  if (desc.task_shader) {
    auto* internal = to_internal(desc.task_shader);
    render_pipeline_desc->setVertexFunction(internal->function.get());
  }
  if (desc.fragment_shader) {
    auto* internal = to_internal(desc.fragment_shader);
    render_pipeline_desc->setFragmentFunction(internal->function.get());
  }

  for (uint32_t i = 0; i < renderpass_info.rt_count; ++i) {
    auto* attachment = render_pipeline_desc->colorAttachments()->object(i);
    attachment->setPixelFormat(to_metal_format(renderpass_info.rt_formats[i]));
  }

  if (renderpass_info.ds_format != rhi::Format::Unknown) {
    const MTL::PixelFormat ds = to_metal_format(renderpass_info.ds_format);
    render_pipeline_desc->setDepthAttachmentPixelFormat(ds);
  }

  if (renderpass_info.sample_count > 1) {
    render_pipeline_desc->setRasterSampleCount(renderpass_info.sample_count);
  }

  NS::Error* error{};
  NS::SharedPtr<MTL::RenderPipelineState> pso =
      NS::TransferPtr(device_->newRenderPipelineState(render_pipeline_desc.get(), &error));
  if (error) {
    auto* err_desc = error->localizedDescription();
    LERROR("{}", err_desc->utf8String());
    error->release();
    return nullptr;
  }

  auto [it, inserted] = pipelines_cache_.emplace(hash, std::move(pso));
  ASSERT(inserted);
  return it->second.get();
}

rhi::CmdEncoder* MetalDevice::begin_cmd_encoder() {
  auto& frame = curr_frame();
  frame.cmd_allocator->reset();
  frame.cmd_buf->beginCommandBuffer(frame.cmd_allocator.get());
  frame.cmd_buffers_allocated++;
  return &frame.cmd_encoder.value();
}

void MetalDevice::end_cmd_encoder([[maybe_unused]] rhi::CmdEncoder* encoder) {
  curr_frame().cmd_buf->endCommandBuffer();
}

void MetalDevice::submit_queue() {
  auto& frame = curr_frame();

  ASSERT(frame.cmd_buffers_allocated == 1);
  frame.cmd_buffers_allocated = 0;

  MTL4::CommandBuffer* submit_cmd_buffers[] = {
      frame.cmd_buf.get(),
  };

  for (auto& drawable : frame.cmd_encoder->presents_) {
    queue_->wait(drawable.get());
  }

  queue_->commit(submit_cmd_buffers, ARRAY_SIZE(submit_cmd_buffers));

  for (auto& drawable : frame.cmd_encoder->presents_) {
    queue_->signalDrawable(drawable.get());
    drawable->present();
  }
  frame.cmd_encoder->presents_.clear();

  // signal fence for the frame after work is complete
  frame.fence_value++;
  queue_->signalEvent(frame.fence.get(), frame.fence_value);

  frame_num_++;

  // wait for the fence from FRAMES_IN_FLIGHT frames ago
  frame = curr_frame();
  frame.fence->waitUntilSignaledValue(frame.fence_value, UINT64_MAX);
}

}  // namespace gfx::metal
