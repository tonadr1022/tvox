#include "MetalDevice.hpp"

#include "core/Assert.hpp"
#include "core/Logger.hpp"
#include "core/Util.hpp"
#include "graphics/metal/MetalTypes.hpp"
#include "graphics/metal/MetalWindow.hpp"
#include "graphics/rhi/CmdEncoder.hpp"
#include "graphics/rhi/Device.hpp"
#include "graphics/rhi/Graphics.hpp"

#include <Foundation/NSSharedPtr.hpp>
#include <Metal/MTL4CommandQueue.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLHeap.hpp>
#include <Metal/MTLPixelFormat.hpp>
#include <Metal/MTLResidencySet.hpp>
#include <Metal/MTLResource.hpp>
#include <Metal/MTLTexture.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <array>
#include <string>

namespace gfx::metal {
namespace {

MTL::PixelFormat to_metal_format(rhi::Format format) {
  switch (format) {
  case rhi::Format::Bgra8Unorm:
    return MTL::PixelFormatBGRA8Unorm;
  case rhi::Format::D32Float:
    return MTL::PixelFormatDepth32Float;
  case rhi::Format::Rgba32Float:
    return MTL::PixelFormatRGBA32Float;
  case rhi::Format::Unknown:
    return MTL::PixelFormatInvalid;
  }
  return MTL::PixelFormatInvalid;
}

MTL::TextureType to_metal(rhi::TextureType type) {
  switch (type) {
  case rhi::TextureType::D1:
    return MTL::TextureType1D;
  case rhi::TextureType::D2:
    return MTL::TextureType2D;
  case rhi::TextureType::D3:
    return MTL::TextureType3D;
    break;
  }
}

MTL::TextureUsage to_metal(rhi::TextureUsage usage) {
  MTL::TextureUsage mtl_usage{};
  if (usage & rhi::TextureUsage::RenderTarget) {
    mtl_usage |= MTL::TextureUsageRenderTarget;
  }
  if (usage & rhi::TextureUsage::RenderTarget) {
    mtl_usage |= MTL::TextureUsageShaderRead;
  }
  if (usage & rhi::TextureUsage::ShaderWrite) {
    mtl_usage |= MTL::TextureUsageShaderWrite;
  }
  return mtl_usage;
}

NS::SharedPtr<NS::String> make_ns_string(const char* str) {
  return NS::TransferPtr(NS::String::string(str, NS::UTF8StringEncoding));
}

} // namespace

MetalDevice::~MetalDevice() = default;

void MetalDevice::init(const rhi::DeviceInitInfo& init_info) {
  device_ = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

  frames_in_flight_ = rhi::k_max_frames_in_flight;

  for (uint32_t i = 0; i < init_info.desired_generic_queue_count; i++) {
    auto queue_index = queues_.size();
    queues_.emplace_back(NS::TransferPtr(device_->newMTL4CommandQueue()));
    caps_.general_queue_indices.push_back(queue_index);
  }
  for (uint32_t i = 0; i < init_info.desired_compute_only_queue_count; i++) {
    auto queue_index = queues_.size();
    queues_.emplace_back(NS::TransferPtr(device_->newMTL4CommandQueue()));
    caps_.general_queue_indices.push_back(queue_index);
  }
  for (uint32_t i = 0; i < init_info.desired_copy_only_queue_count; i++) {
    auto queue_index = queues_.size();
    queues_.emplace_back(NS::TransferPtr(device_->newMTL4CommandQueue()));
    caps_.general_queue_indices.push_back(queue_index);
  }

  NS::Error* error{};
  auto residency_set_descriptor = NS::TransferPtr(MTL::ResidencySetDescriptor::alloc()->init());
  residency_set_descriptor->setInitialCapacity(256);
  residency_set_descriptor->setLabel(make_ns_string("main_residency_set").get());

  main_residency_set_ =
      NS::TransferPtr(device_->newResidencySet(residency_set_descriptor.get(), &error));
  ASSERT(!error);

  for (auto& queue : queues_) {
    queue->addResidencySet(main_residency_set_.get());
  }

  {
    auto tex_view_pool_desc = NS::TransferPtr(MTL::ResourceViewPoolDescriptor::alloc()->init());
    // TODO: improve
    tex_view_pool_desc->setResourceViewCount(512);
    tex_desc_heap_.tex_view_pool =
        NS::TransferPtr(device_->newTextureViewPool(tex_view_pool_desc.get(), &error));
    ASSERT(!error);
    auto base_resource_id = tex_desc_heap_.tex_view_pool->baseResourceID();
    tex_desc_heap_.base_id_buf = NS::TransferPtr(device_->newBuffer(
        &base_resource_id, sizeof(base_resource_id), MTL::ResourceStorageModeShared));
    make_resident(tex_desc_heap_.base_id_buf.get());
  }
}

void MetalDevice::create_swapchain(const rhi::SwapchainDesc& desc, SDL_Window* window,
                                   rhi::Swapchain& swapchain) {
  swapchain.desc = desc;
  swapchain.desc.format = rhi::Format::Bgra8Unorm;

  if (!swapchain.internal_data) {
    swapchain.internal_data = wi::allocator::make_shared<SwapchainMetal>();
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

    for (auto& queue : queues_) {
      queue->addResidencySet(layer->residencySet());
    }
    set_layer_for_window(window, layer);
  }

  auto* layer = internal_data->layer.get();
  layer->setDrawableSize(CGSize{
      .width = static_cast<CGFloat>(desc.width),
      .height = static_cast<CGFloat>(desc.height),
  });
}

bool MetalDevice::create_shader(rhi::ShaderType type, const void* data, uint64_t size,
                                rhi::Shader& shader, std::string_view entry_point) {
  bool success{true};
  shader.internal_data.reset();
  shader.internal_data = wi::allocator::make_shared<ShaderMetal>();
  ShaderMetal* internal_data = to_internal(shader);

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
  }

  return success;
}

bool MetalDevice::create_texture(rhi::Texture& tex, const rhi::TextureDesc& desc) {
  tex.internal_data.reset();
  tex.internal_data = wi::allocator::make_shared<TextureMetal>();
  tex.desc = desc;

  auto* internal_data = to_internal(tex);
  auto tex_desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
  tex_desc->setTextureType(to_metal(desc.type));
  tex_desc->setWidth(desc.width);
  tex_desc->setHeight(desc.height);
  tex_desc->setDepth(desc.depth);
  tex_desc->setPixelFormat(to_metal_format(desc.format));
  tex_desc->setAllowGPUOptimizedContents(true);
  tex_desc->setArrayLength(desc.array_length);
  tex_desc->setMipmapLevelCount(desc.mip_count);
  tex_desc->setUsage(to_metal(desc.usage));

  internal_data->texture = NS::TransferPtr(device_->newTexture(tex_desc.get()));
  make_resident(internal_data->texture.get());

  return true;
}

void MetalDevice::destroy_texture(rhi::Texture& texture) {
  auto* internal_data = to_internal(texture);
  remove_resident(internal_data->texture.get());
  texture.internal_data.reset();
}

bool MetalDevice::create_pipeline(const rhi::PipelineDesc& desc, rhi::Pipeline& pipeline) {
  pipeline.desc = desc;
  pipeline.internal_data.reset();
  pipeline.internal_data = wi::allocator::make_shared<PipelineMetal>();
  // TODO: wtf are we doing here.
  pipelines_cache_.clear();
  return true;
}

bool MetalDevice::create_gpu_heap(const rhi::GpuHeapDesc& desc, rhi::GpuHeap& heap) {
  heap.internal_data.reset();
  heap.internal_data = wi::allocator::make_shared<GpuHeapMetal>();
  auto* internal_data = to_internal(heap);

  MTL::ResourceOptions options{};
  switch (desc.memory_type) {
  case rhi::MemoryType::GpuOnly:
    options |= MTL::ResourceStorageModePrivate;
    break;
  case rhi::MemoryType::CpuVisible:
  case rhi::MemoryType::Readback:
    options |= MTL::ResourceStorageModeShared;
    break;
  }
  options |= MTL::ResourceOptionCPUCacheModeDefault;

  MTL::SizeAndAlign requirements = device_->heapBufferSizeAndAlign(desc.size, options);
  if (!requirements.size || !requirements.align) {
    LERROR("GPU heap size and alignment requuirements insufficient");
    return false;
  }

  auto heap_descriptor = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
  heap_descriptor->setSize(align_up(desc.size, requirements.align));
  heap_descriptor->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);
  // TODO: maybe support auto, depends on workload
  heap_descriptor->setType(MTL::HeapTypePlacement);
  heap_descriptor->setResourceOptions(options);

  internal_data->heap = NS::TransferPtr(device_->newHeap(heap_descriptor.get()));
  internal_data->buffer = NS::TransferPtr(internal_data->heap->newBuffer(desc.size, options, 0));
  make_resident(internal_data->buffer.get()); // TODO: heap for textures?

  if (desc.memory_type != rhi::MemoryType::GpuOnly) {
    heap.range.cpu = reinterpret_cast<std::byte*>(internal_data->buffer->contents());
  } else {
    heap.range.cpu = nullptr;
  }
  heap.range.gpu = reinterpret_cast<std::byte*>(internal_data->buffer->gpuAddress());
  heap.range.size = desc.size;

  return true;
}

bool MetalDevice::create_pipeline(const rhi::Shader& compute_shader, rhi::Pipeline& pipeline) {
  pipeline.desc = {};
  pipeline.internal_data.reset();
  pipeline.internal_data = wi::allocator::make_shared<PipelineMetal>();

  auto* internal_data = to_internal(pipeline);

  auto* compute_shader_internal_data = to_internal(compute_shader);

  NS::Error* error{};
  internal_data->compute_pipeline = NS::TransferPtr(
      device_->newComputePipelineState(compute_shader_internal_data->function.get(), &error));

  if (error) {
    auto* desc = error->localizedDescription();
    LOG_ASSERT(0, "{}", desc->utf8String());
    error->release();
    return false;
  }
  return true;
}

MTL::RenderPipelineState*
MetalDevice::get_or_create_render_pipeline(rhi::Pipeline& pipeline,
                                           const rhi::RenderPassInfo& renderpass_info) {
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

rhi::CmdEncoder* MetalDevice::begin_cmd_encoder(rhi::CmdPool& pool) {
  auto* internal_data = to_internal(pool);
  // device_->newCOmmandBuffer();
  // auto& buffer = pool

  MetalCmdEncoder* enc{};
  if (!internal_data->free_encoders.empty()) {
    auto encoder = std::move(internal_data->free_encoders.front());
    internal_data->free_encoders.pop_front();
    enc = internal_data->encoders.emplace_back(std::move(encoder)).get();
  }
  {
    enc = internal_data->encoders.emplace_back(std::make_unique<MetalCmdEncoder>()).get();
    enc->device_ = this;
    enc->cmd_buf_ = NS::TransferPtr(device_->newCommandBuffer());
    enc->queue_index = internal_data->queue_index;

    // TODO: should this be per-command buffer, what are the tradeoffs, what are apple docs
    enc->cmd_allocator_ = internal_data->cmd_allocator;

    auto arg_table_desc = NS::TransferPtr(MTL4::ArgumentTableDescriptor::alloc()->init());
    arg_table_desc->setInitializeBindings(true);
    arg_table_desc->setLabel(NS::String::string("arg_table", NS::UTF8StringEncoding));
    arg_table_desc->setMaxBufferBindCount(3);
    NS::Error* error{};
    enc->arg_table_ = NS::TransferPtr(device_->newArgumentTable(arg_table_desc.get(), &error));
    enc->arg_table_->setAddress(tex_desc_heap_.base_id_buf->gpuAddress(),
                                k_descriptor_heap_arg_table_index);
    ASSERT(!error);
  }

  enc->pending_barriers_.clear();
  ASSERT(enc->cmd_buf_);
  ASSERT(enc->cmd_allocator_);
  enc->cmd_buf_->beginCommandBuffer(enc->cmd_allocator_.get());
  return enc;
}

void MetalDevice::end_cmd_encoder([[maybe_unused]] rhi::CmdEncoder* encoder) {
  auto* mtl_enc = reinterpret_cast<MetalCmdEncoder*>(encoder);
  mtl_enc->end_compute();
  mtl_enc->pending_barriers_.clear();
  mtl_enc->cmd_buf_->endCommandBuffer();
}

void MetalDevice::submit(std::span<rhi::CmdEncoder*> cmds, rhi::TimelinePoint& timeline_point) {
  std::array<std::vector<MTL4::CommandBuffer*>, k_max_queue_count> submit_cmd_buffers_per_queue;
  for (auto* enc : cmds) {
    auto* mtl_enc = reinterpret_cast<MetalCmdEncoder*>(enc);
    uint32_t queue_index = mtl_enc->queue_index;
    auto& queue = queues_[queue_index];
    submit_cmd_buffers_per_queue[queue_index].emplace_back(mtl_enc->cmd_buf_.get());

    for (auto& drawable : mtl_enc->presents_) {
      queue->wait(drawable.get());
    }
  }

  for (size_t i = 0; i < k_max_queue_count; i++) {
    auto& submit_cmd_buffers = submit_cmd_buffers_per_queue[i];
    if (!submit_cmd_buffers.empty()) {
      auto& queue = queues_[i];
      queue->commit(submit_cmd_buffers.data(), submit_cmd_buffers.size());
    }
  }

  for (auto* enc : cmds) {
    auto* mtl_enc = reinterpret_cast<MetalCmdEncoder*>(enc);

    for (auto& drawable : mtl_enc->presents_) {
      auto& queue = queues_[mtl_enc->queue_index];
      queue->signalDrawable(drawable.get());
      drawable->present();
    }
    mtl_enc->presents_.clear();
  }

  auto* timeline_sema_internal_data = to_internal(timeline_point.sema);
  for (auto& queue : queues_) {
    queue->signalEvent(timeline_sema_internal_data->event.get(), timeline_point.value);
  }
}

void MetalDevice::make_resident(MTL::Allocation* alloc) {
  main_residency_set_->addAllocation(alloc);
  main_residency_set_->commit();
}

void MetalDevice::remove_resident(MTL::Allocation* alloc) {
  main_residency_set_->removeAllocation(alloc);
  main_residency_set_->commit();
}

bool MetalDevice::create_buffer(const rhi::BufferDesc& desc, rhi::Buffer& buffer) {
  buffer.internal_data.reset();
  buffer.internal_data = wi::allocator::make_shared<BufferMetal>();
  auto* internal_data = to_internal(buffer);
  ASSERT(desc.size > 0);

  MTL::ResourceOptions options{};
  switch (desc.memory_type) {
  case gfx::rhi::MemoryType::CpuVisible:
  case gfx::rhi::MemoryType::Readback:
    options |= MTL::ResourceStorageModeShared;
    break;
  case gfx::rhi::MemoryType::GpuOnly:
    options |= MTL::ResourceStorageModePrivate;
    break;
  }

  options |= MTL::ResourceOptionCPUCacheModeDefault;

  auto mtl_buffer = NS::TransferPtr(device_->newBuffer(desc.size, options));
  buffer.desc = desc;
  internal_data->buffer = mtl_buffer;
  make_resident(internal_data->buffer.get());

  if (desc.memory_type == rhi::MemoryType::CpuVisible ||
      desc.memory_type == rhi::MemoryType::Readback) {
    buffer.cpu_ptr = mtl_buffer->contents();
  } else {
    buffer.cpu_ptr = nullptr;
  }
  buffer.gpu_ptr = reinterpret_cast<void*>(mtl_buffer->gpuAddress());

  return true;
}

void MetalDevice::destroy_buffer(rhi::Buffer& buffer) {
  auto* internal_data = to_internal(buffer);
  remove_resident(internal_data->buffer.get());
  buffer.internal_data.reset();
}

bool MetalDevice::create_cmd_pool(rhi::CmdPool& pool, uint32_t queue_index) {
  pool.internal_data.reset();
  pool.internal_data = wi::allocator::make_shared<CmdPoolMetal>();
  auto* internal_data = to_internal(pool);
  internal_data->queue_index = queue_index;
  internal_data->cmd_allocator = NS::TransferPtr(device_->newCommandAllocator());
  return true;
}

void MetalDevice::reset_cmd_pool(rhi::CmdPool& pool) {
  auto* internal_data = to_internal(pool);
  for (auto& enc : internal_data->encoders) {
    // TODO: do some assertions here
    internal_data->free_encoders.push_back(std::move(enc));
  }
  internal_data->encoders.clear();
}

void MetalDevice::write_texture_desc(const rhi::TextureDescriptorDesc& desc, rhi::Texture& texture,
                                     uint32_t index) {

  auto tex_view_desc = NS::TransferPtr(MTL::TextureViewDescriptor::alloc()->init());
  // TODO: set the type
  tex_view_desc->setTextureType(to_metal(desc.texture_type));
  tex_view_desc->setLevelRange(
      NS::Range::Make(desc.mip, desc.mip_count == 0 ? texture.desc.mip_count : desc.mip_count));
  tex_view_desc->setSliceRange(NS::Range::Make(
      desc.slice, desc.array_length == 0 ? texture.desc.array_length : desc.array_length));

  auto* tex_internal_data = to_internal(texture);
  tex_view_desc->setPixelFormat(tex_internal_data->texture.get()->pixelFormat());
  tex_desc_heap_.tex_view_pool->setTextureView(tex_internal_data->texture.get(),
                                               tex_view_desc.get(), index);
}

void MetalDevice::create_timeline_semaphore(rhi::TimelineSemaphore& sema) {
  sema.internal_data.reset();
  sema.internal_data = wi::allocator::make_shared<TimelineSemaphoreMetal>();
  auto* internal_data = to_internal(sema);
  internal_data->event = NS::TransferPtr(device_->newSharedEvent());
}

void MetalDevice::wait_timeline_semaphore(rhi::TimelineSemaphore& sema, uint64_t value) {
  auto* internal_data = to_internal(sema);
  internal_data->event->waitUntilSignaledValue(value, UINT64_MAX);
}

} // namespace gfx::metal
