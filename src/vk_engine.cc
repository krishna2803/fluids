#include <chrono>
#include <memory>
#include <thread>

#include "VkBootstrap.h"
#include "logger.hh"
#include "vk_engine.hh"
#include "vk_images.hh"
#include "vk_types.hh"

constexpr bool bUseValidationLayers = true;

static std::unique_ptr<VulkanEngine> loaded_engine = nullptr;

VulkanEngine &VulkanEngine::Get() {
  return *(loaded_engine ? loaded_engine
                         : (loaded_engine = std::unique_ptr<VulkanEngine>(
                                new VulkanEngine())));
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
      LOG_INFO_MSG("Frame Count {}", frame_count);

    draw();
    frame_count++;

    // if (frame_count == 10) {
    //   glfwSetWindowShouldClose(window, true);
    // }
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

  VkPhysicalDeviceVulkan13Features features13{};
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features13.pNext = nullptr;
  features13.dynamicRendering = true;
  features13.synchronization2 = true;

  VkPhysicalDeviceVulkan12Features features12{};
  features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features12.pNext = nullptr;
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

  graphics_queue = vkb_dev.get_queue(vkb::QueueType::graphics).value();
  graphics_queue_family =
      vkb_dev.get_queue_index(vkb::QueueType::graphics).value();

  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties(phys_dev, &device_properties);

  LOG_INFO_MSG("VkPhysicalDeviceLimits::maxMemoryAllocationCount = {}",
               device_properties.limits.maxMemoryAllocationCount);

  LOG_INFO_MSG("Vulkan Initialized");

  VmaAllocatorCreateInfo vma_info{};
  vma_info.physicalDevice = phys_dev;
  vma_info.device = device;
  vma_info.instance = instance;
  vma_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&vma_info, &vma);

  dqueue.push_func([&]() { vmaDestroyAllocator(vma); });
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
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_min_image_count(2)
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

  VkExtent3D draw_img_extent = {window_extent.width, window_extent.height, 1};

  // hardcoding the draw format to 32 bit float
  draw_img.img_fmt = VK_FORMAT_R16G16B16A16_SFLOAT;
  draw_img.img_extent = draw_img_extent;

  VkImageUsageFlags draw_img_use_flags{};
  draw_img_use_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  draw_img_use_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  draw_img_use_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  draw_img_use_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo rimg_info{};
  rimg_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  rimg_info.pNext = nullptr;

  rimg_info.imageType = VK_IMAGE_TYPE_2D;
  rimg_info.format = draw_img.img_fmt;
  rimg_info.extent = draw_img_extent;
  rimg_info.mipLevels = 1;
  rimg_info.arrayLayers = 1;

  // for MSAA. we will not be using it by default, so default it to 1 sample per
  // pixel.
  rimg_info.samples = VK_SAMPLE_COUNT_1_BIT;

  // optimal tiling, which means the image is stored on the best gpu format
  rimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  rimg_info.usage = draw_img_use_flags;

  // for the draw image, we want to allocate it from gpu local memory
  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VK_CHECK(vmaCreateImage(vma, &rimg_info, &rimg_allocinfo, &draw_img.img,
                          &draw_img.allocation, nullptr));

  // build a image-view for the draw image to use for rendering
  VkImageViewCreateInfo rview_info{};
  rview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  rview_info.pNext = nullptr;

  rview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  rview_info.image = draw_img.img;
  rview_info.format = draw_img.img_fmt;
  rview_info.subresourceRange.baseMipLevel = 0;
  rview_info.subresourceRange.levelCount = 1;
  rview_info.subresourceRange.baseArrayLayer = 0;
  rview_info.subresourceRange.layerCount = 1;
  rview_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

  VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &draw_img.img_view));

  dqueue.push_func([=, this]() {
    vkDestroyImageView(device, draw_img.img_view, nullptr);
    vmaDestroyImage(vma, draw_img.img, draw_img.allocation);
  });
}

auto VulkanEngine::destroy_swapchain() -> void {
  vkDestroySwapchainKHR(device, swapchain, nullptr);

  for (auto &img_view : swapchain_image_views) {
    vkDestroyImageView(device, img_view, nullptr);
  }
}

auto VulkanEngine::init_commands() -> void {
  // placeholder
  VkCommandPoolCreateInfo cmd_pool_info{};
  cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmd_pool_info.pNext = nullptr;
  cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cmd_pool_info.queueFamilyIndex = graphics_queue_family;

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateCommandPool(device, &cmd_pool_info, nullptr,
                                 &frames[i].cmd_pool));
    VkCommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext = nullptr;
    cmd_alloc_info.commandPool = frames[i].cmd_pool;
    cmd_alloc_info.commandBufferCount = 1;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VK_CHECK(
        vkAllocateCommandBuffers(device, &cmd_alloc_info, &frames[i].cmd_buf));
  }

  LOG_INFO_MSG("Commands Initialized");
}

auto VulkanEngine::init_sync_structures() -> void {
  // create syncronization structures
  // one fence to control when the gpu has finished rendering the frame,
  // and 2 semaphores to syncronize rendering with swapchain
  // we want the fence to start signalled so we can wait on it on the first

  VkFenceCreateInfo fence_create_info{};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.pNext = nullptr;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkSemaphoreCreateInfo semaphore_create_info{};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_create_info.pNext = nullptr;
  // info.flags = 0;

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr,
                           &frames[i].render_fence));

    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                               &frames[i].swapchain_semaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                               &frames[i].render_semaphore));
  }

  LOG_INFO_MSG("Sync Structures Initialized");
}

auto VulkanEngine::draw_background(VkCommandBuffer cmd) -> void {
  // make a clear-color from frame number. This will flash with a 120 frame
  // period.
  VkClearColorValue clear_value;
  float flash = (1.0f + std::sin(frame_number / 120.0f)) * 0.5f;
  clear_value = {{flash, 0.0f, 0.0f, 1.0f}};

  VkImageSubresourceRange clear_range{};
  clear_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  clear_range.baseMipLevel = 0;
  clear_range.levelCount = VK_REMAINING_MIP_LEVELS;
  clear_range.baseArrayLayer = 0;
  clear_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

  // clear image
  vkCmdClearColorImage(cmd, draw_img.img, VK_IMAGE_LAYOUT_GENERAL, &clear_value,
                       1, &clear_range);
}

auto VulkanEngine::draw() -> void {
  VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().render_fence, true,
                           1'000'000'000U /*ns*/));

  get_current_frame().dqueue.flush();

  VK_CHECK(vkResetFences(device, 1, &get_current_frame().render_fence));

  u32 swapchain_image_idx;
  VK_CHECK(vkAcquireNextImageKHR(device, swapchain, 1'000'000'000U /*ns*/,
                                 get_current_frame().swapchain_semaphore,
                                 nullptr, &swapchain_image_idx));

  VkCommandBuffer cmd = get_current_frame().cmd_buf;
  VK_CHECK(vkResetCommandBuffer(cmd, 0));

  VkCommandBufferBeginInfo cmd_buf_beg_info{};
  cmd_buf_beg_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buf_beg_info.pNext = nullptr;
  cmd_buf_beg_info.pInheritanceInfo = nullptr;
  cmd_buf_beg_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  draw_extent.width = window_extent.width;
  draw_extent.height = window_extent.height;

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_buf_beg_info));

  // transition our main draw image into general layout so we can write into it
  // we will overwrite it all so we dont care about what was the older layout
  vkutil::transition_image(cmd, draw_img.img, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_GENERAL);

  draw_background(cmd);

  // transition the draw image and the swapchain image into their correct
  // transfer layouts
  vkutil::transition_image(cmd, draw_img.img, VK_IMAGE_LAYOUT_GENERAL,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  vkutil::transition_image(cmd, swapchain_images[swapchain_image_idx],
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // execute a copy from the draw image into the swapchain
  vkutil::copy_image_to_image(cmd, draw_img.img,
                              swapchain_images[swapchain_image_idx],
                              draw_extent, swapchain_extent);

  // set swapchain image layout to Present so we can show it on the screen
  vkutil::transition_image(cmd, swapchain_images[swapchain_image_idx],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // finalize the command buffer (we can no longer add commands, but it can
  // now be executed)
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmd_buf_sub_info;
  cmd_buf_sub_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmd_buf_sub_info.pNext = nullptr;
  cmd_buf_sub_info.commandBuffer = cmd;
  cmd_buf_sub_info.deviceMask = 0;

  VkSemaphoreSubmitInfo wait_info{};
  wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  wait_info.pNext = nullptr;
  wait_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
  wait_info.semaphore = get_current_frame().swapchain_semaphore;
  wait_info.deviceIndex = 0;
  wait_info.value = 1;

  VkSemaphoreSubmitInfo signal_info{};
  signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signal_info.pNext = nullptr;
  signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
  signal_info.semaphore = get_current_frame().render_semaphore;
  signal_info.deviceIndex = 0;
  signal_info.value = 1;

  VkSubmitInfo2 sub_info = {};
  sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  sub_info.pNext = nullptr;

  sub_info.waitSemaphoreInfoCount = 1;
  sub_info.pWaitSemaphoreInfos = &wait_info;

  sub_info.signalSemaphoreInfoCount = 1;
  sub_info.pSignalSemaphoreInfos = &signal_info;

  sub_info.commandBufferInfoCount = 1;
  sub_info.pCommandBufferInfos = &cmd_buf_sub_info;

  // submit command buffer to the queue and execute it.
  // render_fence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &sub_info,
                          get_current_frame().render_fence));

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = nullptr;
  present_info.pSwapchains = &swapchain;
  present_info.swapchainCount = 1;

  present_info.pWaitSemaphores = &get_current_frame().render_semaphore;
  present_info.waitSemaphoreCount = 1;

  present_info.pImageIndices = &swapchain_image_idx;
  VK_CHECK(vkQueuePresentKHR(graphics_queue, &present_info));
  frame_number++;
}

auto VulkanEngine::cleanup() -> void {
  LOG_DEBUG_MSG("Cleaning up");
  if (!is_initialized) {
    LOG_WARNING_MSG("::cleanup() called with uininitialised instance!");
    return;
  }

  vkDeviceWaitIdle(device);

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    vkDestroyCommandPool(device, frames[i].cmd_pool, nullptr);

    vkDestroyFence(device, frames[i].render_fence, nullptr);
    vkDestroySemaphore(device, frames[i].render_semaphore, nullptr);
    vkDestroySemaphore(device, frames[i].swapchain_semaphore, nullptr);

    frames[i].dqueue.flush();
  }

  dqueue.flush();

  // VkQueue-s also can’t be destroyed, as, like with the VkPhysicalDevice,
  // they aren’t really created objects, more like a handle to something that
  // already exists as part of the VkInstance.

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
