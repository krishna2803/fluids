#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;

#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "logger.hh"

template <typename T> constexpr T implicit_cast(std::type_identity_t<T> x) {
  return x;
}

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    vk::Result err = x;                                                        \
    if (err != vk::Result::eSuccess) {                                         \
      LOG_ERROR_MSG("Detected Vulkan error: {}", vk::to_string(err));          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define VK_CHECK_THROW(x)                                                      \
  do {                                                                         \
    try {                                                                      \
      x;                                                                       \
    } catch (const vk::SystemError &e) {                                       \
      LOG_ERROR_MSG("Vulkan exception: {}", e.what());                         \
      abort();                                                                 \
    }                                                                          \
  } while (0)
