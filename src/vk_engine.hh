#pragma once

#include "vk_types.hh"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void create_swapchain(u32 width, u32 height);
  void destroy_swapchain();
};
