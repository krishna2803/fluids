#pragma once

#include "vk_descriptors.hh"
#include "vk_mem_alloc.h"
#include "vk_types.hh"
#include "vulkan/vulkan.hpp"

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
    for (const auto &fn : std::views::reverse(deletors))
      fn();
    deletors.clear();
  }
};

struct FrameData {
  vk::CommandPool cmd_pool;
  vk::CommandBuffer cmd_buf;
  vk::Semaphore swapchain_semaphore;
  vk::Semaphore render_semaphore;
  vk::Fence render_fence;
  DelQueue del_queue;
};

struct AllocatedImage {
  vk::Image img;
  vk::ImageView img_view;
  vk::Extent3D img_extent;
  vk::Format img_fmt;
  VmaAllocation allocation;
};

struct ComputePushConstants {
  glm::vec4 data1;
  glm::vec4 data2;
  glm::vec4 data3;
  glm::vec4 data4;
};

struct ComputeEffect {
  std::string_view name;

  vk::Pipeline pipeline;
  vk::PipelineLayout layout;

  ComputePushConstants data;
};

struct VulkanEngine {
  long frame_count = 0;

  vk::Extent2D window_extent{800, 600};

  // singleton
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
  }

private:
  bool is_initialized = false;
  VulkanEngine() = default;
  GLFWwindow *window;

  vk::Instance instance;
  vk::DebugUtilsMessengerEXT dbg_msngr;
  vk::PhysicalDevice gpu;
  vk::Device device;
  vk::SurfaceKHR surface;
  vk::SwapchainKHR swapchain;
  vk::Format swapchain_img_fmt;
  std::vector<vk::Image> swapchain_images;
  std::vector<vk::ImageView> swapchain_image_views;
  vk::Extent2D swapchain_extent;

  AllocatedImage draw_img;
  vk::Extent2D draw_extent;

  vk::Queue graphics_queue;
  u32 graphics_queue_family;

  u32 frame_number;
  FrameData frames[FRAME_OVERLAP];

  DelQueue del_queue;
  VmaAllocator vma;

  DescriptorAllocator global_desc_allocator;
  vk::DescriptorSet draw_img_descriptors;
  vk::DescriptorSetLayout draw_img_desc_set_layout;

  vk::Pipeline gradient_pipeline;
  vk::PipelineLayout gradient_pipeline_layout;

  // imgui
  vk::Fence imm_fence;
  vk::CommandBuffer imm_cmd_buf;
  vk::CommandPool imm_cmd_pool;
  vk::DescriptorPool imgui_pool;

  std::vector<ComputeEffect> bg_effects;
  int cur_bg_effect = 0;

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void init_descriptors();
  void init_pipelines();
  void init_background_pipelines();
  void create_swapchain(u32 width, u32 height);
  void rebuild_swapchain();
  void draw_background(vk::CommandBuffer cmd);
  void destroy_swapchain();

  void imm_submit(std::function<void(vk::CommandBuffer)> &&func);
  void init_imgui();
  void draw_imgui(vk::CommandBuffer cmd, vk::ImageView target_img_view);
};
