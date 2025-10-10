#pragma once

#include <vulkan/vulkan.h>

namespace vkutil {
void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout cur_layout, VkImageLayout new_layout);
}
