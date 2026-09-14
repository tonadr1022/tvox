#include "MetalCmdEncoder.hpp"

#include <Foundation/NSAutoreleasePool.hpp>
#include <Metal/MTL4RenderPass.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include "MetalDevice.hpp"
#include "MetalTypes.hpp"
#include "core/EAssert.hpp"

namespace gfx::metal {

void MetalCmdEncoder::begin_rendering(rhi::Swapchain& swapchain) {
  auto autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
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
  renderpass_info_ = rhi::RenderPassInfo::from(swapchain.desc);
}

void MetalCmdEncoder::end_rendering() {
  ASSERT(curr_render_encoder_);
  curr_render_encoder_->endEncoding();
  curr_render_encoder_ = nullptr;
  renderpass_info_ = {};
}

void MetalCmdEncoder::bind_pipeline(rhi::Pipeline& pipeline) {
  // TODO: flush state, barriers, etc
  ASSERT(device_);
  ASSERT(curr_render_encoder_);
  ASSERT(renderpass_info_.rt_count > 0);

  MTL::RenderPipelineState* pso = device_->ensure_render_pipeline(pipeline, renderpass_info_);
  ASSERT(pso);
  curr_render_encoder_->setRenderPipelineState(pso);
}

void MetalCmdEncoder::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
                           uint32_t first_instance) {
  ASSERT(curr_render_encoder_);
  curr_render_encoder_->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, first_vertex,
                                       vertex_count, instance_count, first_instance);
}

}  // namespace gfx::metal
