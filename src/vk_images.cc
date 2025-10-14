#include "vk_images.hh"

namespace vkutil {

auto transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout cur_layout, VkImageLayout new_layout)
    -> void {
  VkImageMemoryBarrier2 img_barrier{};
  img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  img_barrier.pNext = nullptr;

  // a bit of performance hit because of VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
  // the GPU will stall a bit
  img_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  img_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
  img_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  img_barrier.dstAccessMask =
      VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

  img_barrier.oldLayout = cur_layout;
  img_barrier.newLayout = new_layout;

  VkImageAspectFlags aspect_mask =
      (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
          ? VK_IMAGE_ASPECT_DEPTH_BIT
          : VK_IMAGE_ASPECT_COLOR_BIT;

  VkImageSubresourceRange sub_res_img{};
  sub_res_img.aspectMask = aspect_mask;
  sub_res_img.baseMipLevel = 0;
  sub_res_img.levelCount = VK_REMAINING_MIP_LEVELS;
  sub_res_img.baseArrayLayer = 0;
  sub_res_img.layerCount = VK_REMAINING_ARRAY_LAYERS;

  img_barrier.subresourceRange = sub_res_img;
  img_barrier.image = image;

  VkDependencyInfo dep_info{};
  dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep_info.pNext = nullptr;
  //   dep_info.memoryBarrierCount = 1;
  dep_info.imageMemoryBarrierCount = 1;
  dep_info.pImageMemoryBarriers = &img_barrier;

  vkCmdPipelineBarrier2(cmd, &dep_info);
  // img_barrier.subresourceRange =
}

auto copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst,
                         VkExtent2D src_sz, VkExtent2D dst_sz) -> void {
  VkImageBlit2 blit_region{};
  blit_region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
  blit_region.pNext = nullptr;

  blit_region.srcOffsets[1].x = src_sz.width;
  blit_region.srcOffsets[1].y = src_sz.height;
  blit_region.srcOffsets[1].z = 1;

  blit_region.dstOffsets[1].x = dst_sz.width;
  blit_region.dstOffsets[1].y = dst_sz.height;
  blit_region.dstOffsets[1].z = 1;

  blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_region.srcSubresource.baseArrayLayer = 0;
  blit_region.srcSubresource.layerCount = 1;
  blit_region.srcSubresource.mipLevel = 0;

  blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_region.dstSubresource.baseArrayLayer = 0;
  blit_region.dstSubresource.layerCount = 1;
  blit_region.dstSubresource.mipLevel = 0;

  VkBlitImageInfo2 blit_info{};
  blit_info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
  blit_info.pNext = nullptr;
  blit_info.srcImage = src;
  blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blit_info.dstImage = dst;
  blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blit_info.filter = VK_FILTER_LINEAR;
  blit_info.regionCount = 1;
  blit_info.pRegions = &blit_region;

  vkCmdBlitImage2(cmd, &blit_info);
}

}; // namespace vkutil
