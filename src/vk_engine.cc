#include <cassert>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>

#include "VkBootstrap.h"
#include "logger.hh"
#include "vk_engine.hh"
#include "vk_initializers.hh"
#include "vk_types.hh"

constexpr bool bUseValidationLayers = false;

static std::unique_ptr<VulkanEngine> loaded_engine = nullptr;

VulkanEngine &VulkanEngine::Get() {
  if (!loaded_engine)
    loaded_engine = std::unique_ptr<VulkanEngine>(new VulkanEngine());
  return *loaded_engine;
}

auto VulkanEngine::init() -> void {
  if (is_initialized) {
    LOG_WARNING_MSG("Engine already initialized.");
    return;
  }

  if (!glfwInit()) {
    LOG_ERROR_MSG("Couldn't initialize GLFW.");
    abort();
  }
  LOG_INFO_MSG("GLFW Initialized");

  if (!glfwVulkanSupported()) {
    glfwTerminate();
    LOG_ERROR_MSG("Vulkan not supported.");
    abort();
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(window_extent.width, window_extent.height,
                            "Vulkan Engine", NULL, NULL);

  LOG_INFO_MSG("GLFW Window Initialized");

  if (!window) {
    glfwTerminate();
    LOG_ERROR_MSG("Couldn't create GLFW window.");
    abort();
  }

  init_vulkan();
  init_swapchain();
  init_commands();
  init_sync_structures();

  is_initialized = true;
}

auto VulkanEngine::draw() -> void {
  //
  // placeholder
  //
}

auto VulkanEngine::run() -> void {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    stop_rendering = glfwGetWindowAttrib(window, GLFW_ICONIFIED);

    // Do not draw if we are minimized
    if (stop_rendering) {
      // Throttle the speed to avoid endless spinning
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, true);
    }

    if (frame_count % 2000 == 0)
      LOG_INFO_MSG("{}", frame_count);

    draw();
    frame_count++;

    if (frame_count == 10000) {
      glfwSetWindowShouldClose(window, true);
    }
  }
}

auto VulkanEngine::init_vulkan() -> void {
  vkb::InstanceBuilder builder;
  auto inst_ret = builder.set_app_name("Example Vulkan Application")
                      .request_validation_layers(bUseValidationLayers)
                      .use_default_debug_messenger()
                      .require_api_version(1, 4, 0)
                      .build();

  vkb::Instance vkb_inst = inst_ret.value();
  instance = vkb_inst.instance;
  dbg_msngr = vkb_inst.debug_messenger;

  surface = VK_NULL_HANDLE;
  VkResult glfw_result =
      glfwCreateWindowSurface(instance, window, nullptr, &surface);
  if (glfw_result != VK_SUCCESS) {
    LOG_ERROR_MSG("Failed to select create window surface. Error: {}",
                  std::to_string(glfw_result));
    glfwDestroyWindow(window);
    glfwTerminate();
    abort();
  }

  VkPhysicalDeviceVulkan13Features features13;
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features13.dynamicRendering = true;
  features13.synchronization2 = true;

  VkPhysicalDeviceVulkan12Features features12;
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  vkb::PhysicalDeviceSelector selector{vkb_inst};
  vkb::PhysicalDevice phys_dev = selector.set_minimum_version(1, 4)
                                     .set_required_features_13(features13)
                                     .set_required_features_12(features12)
                                     .set_surface(surface)
                                     .select()
                                     .value();

  vkb::DeviceBuilder dev_builder{phys_dev};
  vkb::Device vkb_dev = dev_builder.build().value();

  device = vkb_dev.device;
  gpu = vkb_dev.physical_device;

  LOG_INFO_MSG("Vulkan Initialized");
}

auto VulkanEngine::create_swapchain(const u32 width, const u32 height) -> void {
  vkb::SwapchainBuilder builder{gpu, device, surface};
  swapchain_img_fmt = VK_FORMAT_B8G8R8A8_UNORM;
  vkb::Swapchain vkb_swapchain =
      builder
          //.use_default_format_selection()
          .set_desired_format(VkSurfaceFormatKHR{
              .format = swapchain_img_fmt,
              .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          // use vsync present mode
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
          .set_desired_extent(width, height)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build()
          .value();

  swapchain_extent = vkb_swapchain.extent;
  // store swapchain and its related images
  swapchain = vkb_swapchain.swapchain;
  swapchain_images = vkb_swapchain.get_images().value();
  swapchain_image_views = vkb_swapchain.get_image_views().value();
}

auto VulkanEngine::init_swapchain() -> void {
  create_swapchain(window_extent.width, window_extent.height);

  LOG_INFO_MSG("Swapchain Initialized");
}

auto VulkanEngine::destroy_swapchain() -> void {
  vkDestroySwapchainKHR(device, swapchain, nullptr);

  for (auto &img_view : swapchain_image_views) {
    vkDestroyImageView(device, img_view, nullptr);
  }
}

auto VulkanEngine::init_commands() -> void {
  // placeholder
  LOG_INFO_MSG("Commands Initialized");
}

auto VulkanEngine::init_sync_structures() -> void {
  // placeholder
  LOG_INFO_MSG("Sync Structures Initialized");
}

auto VulkanEngine::cleanup() -> void {
  LOG_DEBUG_MSG("Cleaning up");
  if (is_initialized) {

    destroy_swapchain();
    LOG_DEBUG_MSG("Swapchain destroyed");

    vkDestroySurfaceKHR(instance, surface, nullptr);
    LOG_DEBUG_MSG("Surface destroyed");
    vkDestroyDevice(device, nullptr);
    LOG_DEBUG_MSG("Logical Device destroyed");

    // VkPhysicalDevice can’t be destroyed, as it’s not a Vulkan resource
    // per-se, it’s more like just a handle to a GPU in the system

    vkb::destroy_debug_utils_messenger(instance, dbg_msngr);
    LOG_DEBUG_MSG("Debug utils messenger destroyed");

    vkDestroyInstance(instance, nullptr);
    LOG_DEBUG_MSG("VkInstance destroyed");

    glfwDestroyWindow(window);
    LOG_DEBUG_MSG("GLFWWindow destroyed");
    glfwTerminate();
  }
}
