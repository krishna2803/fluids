#include "vk_images.hh"
#include "logger.hh"

namespace vkutil {

auto transition_image(vk::CommandBuffer cmd, vk::Image image,
                      vk::ImageLayout cur_layout, vk::ImageLayout new_layout)
    -> void {
  //   LOG_TRACE("Transitioning image from {} to {}", vk::to_string(cur_layout),
  //             vk::to_string(new_layout));

  // a bit of performance hit because of VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
  // the GPU will stall a bit
  vk::ImageMemoryBarrier2 img_barrier{};
  img_barrier.setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands);
  img_barrier.setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite);
  img_barrier.setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands);
  img_barrier.setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite |
                               vk::AccessFlagBits2::eMemoryRead);
  img_barrier.setOldLayout(cur_layout);
  img_barrier.setNewLayout(new_layout);
  // img_barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
  // img_barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

  vk::ImageAspectFlags aspect_mask =
      (new_layout == vk::ImageLayout::eDepthAttachmentOptimal)
          ? vk::ImageAspectFlagBits::eDepth
          : vk::ImageAspectFlagBits::eColor;

  vk::ImageSubresourceRange subresource_range{};
  subresource_range.setAspectMask(aspect_mask);
  subresource_range.setBaseMipLevel(0);
  subresource_range.setLevelCount(vk::RemainingMipLevels);
  subresource_range.setBaseArrayLayer(0);
  subresource_range.setLayerCount(vk::RemainingArrayLayers);

  img_barrier.setSubresourceRange(subresource_range);
  img_barrier.setImage(image);

  vk::DependencyInfo dep_info{};
  dep_info.setImageMemoryBarrierCount(1);
  dep_info.setPImageMemoryBarriers(&img_barrier);

  cmd.pipelineBarrier2(dep_info);
}

auto copy_image_to_image(vk::CommandBuffer cmd, vk::Image src, vk::Image dst,
                         vk::Extent2D src_sz, vk::Extent2D dst_sz) -> void {
  //   LOG_TRACE("Copying image from {}x{} to {}x{}", src_sz.width,
  //   src_sz.height,
  //             dst_sz.width, dst_sz.height);

  vk::ImageSubresourceLayers src_subresource{};
  src_subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
  src_subresource.setMipLevel(0);
  src_subresource.setBaseArrayLayer(0);
  src_subresource.setLayerCount(1);

  vk::ImageSubresourceLayers dst_subresource{};
  dst_subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
  dst_subresource.setMipLevel(0);
  dst_subresource.setBaseArrayLayer(0);
  dst_subresource.setLayerCount(1);

  vk::ImageBlit2 blit_region{};
  blit_region.setSrcSubresource(src_subresource);
  blit_region.setDstSubresource(dst_subresource);

  std::array<vk::Offset3D, 2> src_offsets = {
      vk::Offset3D{}, // (0,0,0)
      vk::Offset3D{
          static_cast<int32_t>(src_sz.width),  // x
          static_cast<int32_t>(src_sz.height), // y
          1                                    // z
      }};

  std::array<vk::Offset3D, 2> dst_offsets = {
      vk::Offset3D{}, // (0,0,0)
      vk::Offset3D{
          static_cast<int32_t>(dst_sz.width),  // x
          static_cast<int32_t>(dst_sz.height), // y
          1                                    // z
      }};

  blit_region.setSrcOffsets(src_offsets);
  blit_region.setDstOffsets(dst_offsets);

  vk::BlitImageInfo2 blit_info{};
  blit_info.setSrcImage(src);
  blit_info.setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal);
  blit_info.setDstImage(dst);
  blit_info.setDstImageLayout(vk::ImageLayout::eTransferDstOptimal);
  blit_info.setRegionCount(1);
  blit_info.setPRegions(&blit_region);
  blit_info.setFilter(vk::Filter::eLinear);

  cmd.blitImage2(blit_info);
}

} // namespace vkutil
