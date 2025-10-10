#include "vk_images.hh"
#include <vulkan/vulkan_core.h>

namespace vkutil {

void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout cur_layout, VkImageLayout new_layout) {
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

}; // namespace vkutil
