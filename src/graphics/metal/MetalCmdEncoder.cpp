#include "MetalCmdEncoder.hpp"

#include "MetalDevice.hpp"
#include "MetalTypes.hpp"
#include "core/Assert.hpp"
#include "graphics/rhi/Graphics.hpp"

#include <Foundation/NSAutoreleasePool.hpp>
#include <Foundation/NSSharedPtr.hpp>
#include <Foundation/NSString.hpp>
#include <Metal/MTL4ArgumentTable.hpp>
#include <Metal/MTL4CommandEncoder.hpp>
#include <Metal/MTL4RenderPass.hpp>
#include <Metal/MTLCommandEncoder.hpp>
#include <Metal/MTLRenderCommandEncoder.hpp>
#include <Metal/MTLRenderPass.hpp>
#include <Metal/MTLTypes.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

namespace gfx::metal {

namespace {
MTL::LoadAction to_metal(rhi::LoadOp load_op) {
  switch (load_op) {
  case rhi::LoadOp::Load:
    return MTL::LoadActionLoad;
  case rhi::LoadOp::Clear:
    return MTL::LoadActionClear;
  case rhi::LoadOp::DontCare:
    return MTL::LoadActionDontCare;
  }
}

MTL::StoreAction to_metal(rhi::StoreOp store_op) {
  switch (store_op) {
  case rhi::StoreOp::Store:
    return MTL::StoreActionStore;
  case rhi::StoreOp::DontCare:
    return MTL::StoreActionDontCare;
  }
}
} // namespace

MetalCmdEncoder::~MetalCmdEncoder() = default;

void MetalCmdEncoder::begin_rendering(rhi::Swapchain& swapchain) {
  end_compute();
  auto* internal_data = to_internal(swapchain);

  auto* drawable = internal_data->layer->nextDrawable();
  while (!drawable) {
    drawable = internal_data->layer->nextDrawable();
  }

  internal_data->curr_texture = NS::TransferPtr(drawable->texture()->retain());

  presents_.emplace_back(NS::TransferPtr(drawable->retain()));

  NS::SharedPtr<MTL4::RenderPassDescriptor> desc =
      NS::TransferPtr(MTL4::RenderPassDescriptor::alloc()->init());
  desc->setRenderTargetWidth(internal_data->curr_texture->width());
  desc->setRenderTargetHeight(internal_data->curr_texture->height());
  desc->setRenderTargetArrayLength(1);

  auto color_descriptor =
      NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()->init());
  color_descriptor->setTexture(internal_data->curr_texture.get());
  color_descriptor->setClearColor(MTL::ClearColor::Make(0.3, 0.3, 0.7, 1));
  color_descriptor->setLevel(0);
  color_descriptor->setSlice(0);
  color_descriptor->setLoadAction(MTL::LoadActionClear);
  color_descriptor->setStoreAction(MTL::StoreActionStore);

  desc->colorAttachments()->setObject(color_descriptor.get(), 0);

  curr_render_encoder_ = NS::TransferPtr(cmd_buf_->renderCommandEncoder(desc.get())->retain());
  curr_render_encoder_->setArgumentTable(arg_table_.get(),
                                         MTL::RenderStageFragment | MTL::RenderStageMesh |
                                             MTL::RenderStageObject | MTL::RenderStageVertex);
  renderpass_info_ = rhi::RenderPassInfo::from(swapchain.desc);
}

void MetalCmdEncoder::begin_rendering(uint32_t width, uint32_t height,
                                      std::span<rhi::RenderAttInfo> color_atts,
                                      rhi::RenderAttInfo* depth_att) {
  end_compute();
  auto render_pass_desc = NS::TransferPtr(MTL4::RenderPassDescriptor::alloc()->init());
  render_pass_desc->setRenderTargetWidth(width);
  render_pass_desc->setRenderTargetWidth(height);
  render_pass_desc->setRenderTargetArrayLength(1);

  for (size_t color_desc_i = 0; color_desc_i < color_atts.size(); color_desc_i++) {
    auto& color_att = color_atts[color_desc_i];
    auto* tex_internal_data = to_internal(color_att.render_view.tex);
    auto color_desc = NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()->init());
    color_desc->setTexture(tex_internal_data->texture.get());
    color_desc->setClearColor(
        MTL::ClearColor::Make(color_att.clear.color.x, color_att.clear.color.y,
                              color_att.clear.color.z, color_att.clear.color.w));
    color_desc->setLevel(0);
    color_desc->setSlice(0);
    color_desc->setLoadAction(MTL::LoadActionClear);
    color_desc->setStoreAction(MTL::StoreActionStore);

    render_pass_desc->colorAttachments()->setObject(color_desc.get(), color_desc_i);
  }

  if (depth_att) {
    auto* tex_view_internal_data = to_internal(depth_att->render_view.tex);
    auto depth_att_desc =
        NS::TransferPtr(MTL::RenderPassDepthAttachmentDescriptor::alloc()->init());
    depth_att_desc->setLevel(0);
    depth_att_desc->setSlice(0);
    depth_att_desc->setLoadAction(to_metal(depth_att->load_op));
    depth_att_desc->setStoreAction(to_metal(depth_att->store_op));
    depth_att_desc->setTexture(tex_view_internal_data->texture.get());
    depth_att_desc->setClearDepth(depth_att->clear.depth_stencil.depth);
    render_pass_desc->setDepthAttachment(depth_att_desc.get());
  }

  // TODO: do we need retain
  // TODO: share with swapchain code
  curr_render_encoder_ =
      NS::TransferPtr(cmd_buf_->renderCommandEncoder(render_pass_desc.get())->retain());
  curr_render_encoder_->setArgumentTable(arg_table_.get(),
                                         MTL::RenderStageFragment | MTL::RenderStageMesh |
                                             MTL::RenderStageObject | MTL::RenderStageVertex);
  renderpass_info_ = rhi::RenderPassInfo::from(color_atts, depth_att);
}

void MetalCmdEncoder::end_rendering() {
  ASSERT(curr_render_encoder_);
  curr_render_encoder_->endEncoding();
  curr_render_encoder_ = nullptr;
  renderpass_info_ = {};
}

void MetalCmdEncoder::bind_pipeline(rhi::Pipeline& pipeline) {
  auto* internal_data = to_internal(pipeline);
  if (internal_data->compute_pipeline) {
    start_compute();
    curr_compute_encoder_->setComputePipelineState(internal_data->compute_pipeline.get());
  } else {
    ASSERT(curr_render_encoder_);
    ASSERT(renderpass_info_.rt_count > 0);
    MTL::RenderPipelineState* pso =
        device_->get_or_create_render_pipeline(pipeline, renderpass_info_);
    ASSERT(pso);
    curr_render_encoder_->setRenderPipelineState(pso);
  }
}

void MetalCmdEncoder::draw(const void* root, uint32_t vertex_count, uint32_t instance_count,
                           uint32_t first_vertex, uint32_t first_instance) {
  set_root(root);
  ASSERT(curr_render_encoder_);
  curr_render_encoder_->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, first_vertex,
                                       vertex_count, instance_count, first_instance);
}
void MetalCmdEncoder::draw_indexed(const void* root, uint32_t index_count,
                                   rhi::Buffer& index_buffer) {
  set_root(root);
  ASSERT(curr_render_encoder_);
  curr_render_encoder_->drawIndexedPrimitives(
      MTL::PrimitiveTypeTriangle, index_count, MTL::IndexTypeUInt32,
      reinterpret_cast<MTL::GPUAddress>(index_buffer.gpu_ptr), index_buffer.desc.size);
}

void MetalCmdEncoder::dispatch(const void* root, uint32_t thread_groups_x, uint32_t thread_groups_y,
                               uint32_t thread_groups_z) {
  set_root(root);
  curr_compute_encoder_->dispatchThreadgroups(
      // TODO: get the threads per thread group from reflection
      MTL::Size::Make(thread_groups_x, thread_groups_y, thread_groups_z), MTL::Size::Make(1, 1, 1));
}

void MetalCmdEncoder::barrier() {
  if (curr_compute_encoder_) {
    curr_render_encoder_->barrierAfterStages(MTL::StageAll, MTL::StageAll,
                                             MTL4::VisibilityOptionDevice);
  }
}

void MetalCmdEncoder::set_root(const void* root) const {
  ASSERT((reinterpret_cast<uintptr_t>(root) & 15ull) == 0);
  arg_table_->setAddress(reinterpret_cast<uintptr_t>(root), 0);
}

void MetalCmdEncoder::end_compute() {
  if (curr_compute_encoder_) {
    curr_compute_encoder_->endEncoding();
    curr_compute_encoder_ = nullptr;
  }
}

void MetalCmdEncoder::start_compute() {
  ASSERT(!curr_render_encoder_);
  if (!curr_compute_encoder_) {
    curr_compute_encoder_ = NS::TransferPtr(cmd_buf_->computeCommandEncoder());
    curr_compute_encoder_->setArgumentTable(arg_table_.get());
  }
}

void MetalCmdEncoder::copy_buffer_to_buffer(rhi::Buffer& dst, size_t dst_offset, rhi::Buffer& src,
                                            size_t src_offset, size_t size) {
  start_compute();
  auto* src_internal = to_internal(src);
  auto* dst_internal = to_internal(dst);
  curr_compute_encoder_->copyFromBuffer(src_internal->buffer.get(), src_offset,
                                        dst_internal->buffer.get(), dst_offset, size);
}

} // namespace gfx::metal
