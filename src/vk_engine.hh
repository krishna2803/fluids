#pragma once

#include "vk_mem_alloc.h"
#include "vk_types.hh"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr i32 FRAME_OVERLAP = 2;

struct DelQueue {
  std::deque<std::function<void()>> deletors;

  // Doing callbacks like this is inneficient at scale, because we are storing
  // whole std::functions for every object we are deleting, which is not going
  // to be optimal. For the amount of objects we will use in this tutorial, its
  // going to be fine. but if you need to delete thousands of objects and want
  // them deleted faster, a better implementation would be to store arrays of
  // vulkan handles of various types such as VkImage, VkBuffer, and so on. And
  // then delete those from a loop.

  void push_func(std::function<void()> &&func) { deletors.emplace_back(func); }

  void flush() {
    for (const auto &fn : std::views::reverse(deletors)) {
      fn();
    }
    deletors.clear();
  }
};

struct FrameData {
  VkCommandPool cmd_pool;
  VkCommandBuffer cmd_buf;
  VkSemaphore swapchain_semaphore;
  VkSemaphore render_semaphore;
  VkFence render_fence;
  DelQueue dqueue;
};

struct AllocatedImage {
  VkImage img;
  VkImageView img_view;
  VkExtent3D img_extent;
  VkFormat img_fmt;
  VmaAllocation allocation;
};

struct VulkanEngine {
  int frame_count = 0;

  VkExtent2D window_extent{800, 600};

  static VulkanEngine &Get();
  VulkanEngine(const VulkanEngine &) = delete;
  VulkanEngine &operator=(const VulkanEngine &) = delete;
  VulkanEngine(VulkanEngine &&) = delete;
  VulkanEngine &operator=(VulkanEngine &&) = delete;
  ~VulkanEngine() = default;

  void init();
  void cleanup();
  void draw();
  void run();

  FrameData &get_current_frame() {
    return frames[frame_number % FRAME_OVERLAP];
  };

private:
  bool is_initialized = false;
  bool stop_rendering = false;
  VulkanEngine() = default;
  GLFWwindow *window;

  VkInstance instance;
  VkDebugUtilsMessengerEXT dbg_msngr;
  VkPhysicalDevice gpu;
  VkDevice device;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;
  VkFormat swapchain_img_fmt;
  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;
  VkExtent2D swapchain_extent;

  AllocatedImage draw_img;
  VkExtent2D draw_extent;

  VkQueue graphics_queue;
  u32 graphics_queue_family;

  u32 frame_number;
  FrameData frames[FRAME_OVERLAP];

  DelQueue dqueue;
  VmaAllocator vma;

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void create_swapchain(u32 width, u32 height);
  void draw_background(VkCommandBuffer cmd);
  void destroy_swapchain();
};
