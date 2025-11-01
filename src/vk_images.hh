#pragma once

#include <vulkan/vulkan.hpp>

namespace vkutil {
void transition_image(vk::CommandBuffer cmd, vk::Image image,
                      vk::ImageLayout cur_layout, vk::ImageLayout new_layout);

void copy_image_to_image(vk::CommandBuffer cmd, vk::Image src, vk::Image dst,
                         vk::Extent2D src_sz, vk::Extent2D dst_sz);
} // namespace vkutil
