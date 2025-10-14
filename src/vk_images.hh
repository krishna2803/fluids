#pragma once

#include <vulkan/vulkan.h>

namespace vkutil {
void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout cur_layout, VkImageLayout new_layout);

void copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst,
                         VkExtent2D src_sz, VkExtent2D dst_sz);
} // namespace vkutil
