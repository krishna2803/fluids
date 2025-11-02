#pragma once

#include <optional>
#include <string_view>
#include <vulkan/vulkan.hpp>

namespace vkutil {
std::optional<vk::ShaderModule> load_shader_module(std::string_view filename,
                                                   vk::Device &device);
}; // namespace vkutil
