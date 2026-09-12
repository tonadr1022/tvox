#include "MetalCmdEncoder.hpp"

#include <Metal/MTL4RenderPass.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/CAMetalLayer.hpp>

#include "MetalTypes.hpp"
#include "core/EAssert.hpp"

namespace gfx::metal {

void MetalCmdEncoder::begin_rendering(rhi::Swapchain& swapchain) {
  auto* internal_data = to_internal(swapchain);

  auto* drawable = internal_data->layer->nextDrawable();
  while (!drawable) {
    drawable = internal_data->layer->nextDrawable();
  }

  internal_data->curr_drawable = NS::TransferPtr(drawable->retain());
  internal_data->curr_texture = NS::TransferPtr(drawable->texture()->retain());

  presents_.emplace_back(internal_data->curr_drawable);

  // curr_render_encoder = NS::TransferPtr();
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
  color_descriptor->setLoadAction(MTL::LoadActionDontCare);
  color_descriptor->setStoreAction(MTL::StoreActionStore);

  desc->colorAttachments()->setObject(color_descriptor.get(), 0);

  curr_render_encoder_ = NS::TransferPtr(cmd_buf_->renderCommandEncoder(desc.get()));
}

void MetalCmdEncoder::end_rendering() {
  ASSERT(curr_render_encoder_);
  curr_render_encoder_->endEncoding();
  curr_render_encoder_ = nullptr;
}

}  // namespace gfx::metal
