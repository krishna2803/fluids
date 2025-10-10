#pragma once

#include "vk_types.hh"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

struct FrameData {
  VkCommandPool cmd_pool;
  VkCommandBuffer cmd_buf;
  VkSemaphore swapchain_semaphore;
  VkSemaphore render_semaphore;
  VkFence render_fence;
};

constexpr i32 FRAME_OVERLAP = 2;

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

  VkQueue graphics_queue;
  u32 graphics_queue_family;

  u32 frame_number;
  FrameData frames[FRAME_OVERLAP];

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void create_swapchain(u32 width, u32 height);
  void destroy_swapchain();
};
