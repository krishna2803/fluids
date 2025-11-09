#include <cmath>
#include <filesystem>
#include <memory>

#include "VkBootstrap.h"
#include "logger.hh"
#include "vk_descriptors.hh"
#include "vk_engine.hh"
#include "vk_images.hh"
#include "vk_pipelines.hh"
#include "vk_types.hh"
#include "vulkan/vulkan.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace fs = std::filesystem;

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
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
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
  init_descriptors();
  init_pipelines();
  init_imgui();

  is_initialized = true;
}

auto VulkanEngine::run() -> void {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    stop_rendering = glfwGetWindowAttrib(window, GLFW_ICONIFIED);

    // do not draw if we are minimized
    // if (stop_rendering) {
    //   // throttle the speed to avoid endless spinning
    //   std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //   continue;
    // }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    if (frame_count % 2000 == 0)
      LOG_INFO_MSG("Frame Count {}", frame_count);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();

    draw();
    frame_count++;
  }
}

auto VulkanEngine::init_vulkan() -> void {
  LOG_DEBUG_MSG("Initializing Vulkan instance");

  vkb::InstanceBuilder builder;
  auto inst_ret = builder.set_app_name("Example Vulkan Application")
                      .request_validation_layers(bUseValidationLayers)
                      .use_default_debug_messenger()
                      .require_api_version(1, 4, 0)
                      .build();

  vkb::Instance vkb_inst = inst_ret.value();
  instance = implicit_cast<vk::Instance>(vkb_inst.instance);
  dbg_msngr =
      implicit_cast<vk::DebugUtilsMessengerEXT>(vkb_inst.debug_messenger);

  LOG_DEBUG_MSG("Creating window surface");
  VkSurfaceKHR c_surface;
  VkResult glfw_result = glfwCreateWindowSurface(
      implicit_cast<VkInstance>(instance), window, nullptr, &c_surface);
  if (glfw_result != VK_SUCCESS) {
    LOG_ERROR_MSG("Failed to create window surface. GLFW Error: {}",
                  std::to_string(glfw_result));
    glfwDestroyWindow(window);
    glfwTerminate();
    abort();
  }
  surface = vk::SurfaceKHR{c_surface};
  LOG_DEBUG_MSG("Window surface created successfully");

  vk::PhysicalDeviceVulkan13Features features13{};
  features13.setSynchronization2(true);
  features13.setDynamicRendering(true);

  vk::PhysicalDeviceVulkan12Features features12{};
  features12.setDescriptorIndexing(true);
  features12.setBufferDeviceAddress(true);

  auto c_features13 =
      implicit_cast<VkPhysicalDeviceVulkan13Features>(features13);
  auto c_features12 =
      implicit_cast<VkPhysicalDeviceVulkan12Features>(features12);

  LOG_DEBUG_MSG("Selecting physical device");
  vkb::PhysicalDeviceSelector selector{vkb_inst};
  vkb::PhysicalDevice phys_dev =
      selector.set_minimum_version(1, 4)
          .set_required_features_13(c_features13)
          .set_required_features_12(c_features12)
          .set_surface(implicit_cast<VkSurfaceKHR>(surface))
          .select()
          .value();

  LOG_DEBUG_MSG("Creating logical device");
  vkb::DeviceBuilder dev_builder{phys_dev};
  vkb::Device vkb_dev = dev_builder.build().value();

  device = implicit_cast<vk::Device>(vkb_dev.device);
  gpu = static_cast<vk::PhysicalDevice>(vkb_dev.physical_device);

  graphics_queue = implicit_cast<vk::Queue>(
      vkb_dev.get_queue(vkb::QueueType::graphics).value());
  graphics_queue_family =
      vkb_dev.get_queue_index(vkb::QueueType::graphics).value();

  vk::PhysicalDeviceProperties device_properties = gpu.getProperties();

  LOG_INFO_MSG("GPU: {}", device_properties.deviceName.data());

  LOG_INFO_MSG("Vulkan initialized successfully");

  LOG_DEBUG_MSG("Creating VMA allocator");
  VmaAllocatorCreateInfo vma_info{};
  vma_info.physicalDevice = implicit_cast<VkPhysicalDevice>(gpu);
  vma_info.device = implicit_cast<VkDevice>(device);
  vma_info.instance = implicit_cast<VkInstance>(instance);
  vma_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&vma_info, &vma);

  del_queue.push_func([&]() { vmaDestroyAllocator(vma); });
  LOG_DEBUG_MSG("VMA allocator created");
}

auto VulkanEngine::create_swapchain(const u32 width, const u32 height) -> void {
  LOG_DEBUG_MSG("Creating swapchain {}x{}", width, height);

  vkb::SwapchainBuilder builder{implicit_cast<VkPhysicalDevice>(gpu),
                                implicit_cast<VkDevice>(device),
                                implicit_cast<VkSurfaceKHR>(surface)};
  swapchain_img_fmt = vk::Format::eB8G8R8A8Unorm;

  vkb::Swapchain vkb_swapchain =
      builder
          .set_desired_format(VkSurfaceFormatKHR{
              .format = static_cast<VkFormat>(swapchain_img_fmt),
              .colorSpace = static_cast<VkColorSpaceKHR>(
                  vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear)})
          .set_desired_present_mode(
              static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo))
          .set_desired_min_image_count(2)
          .set_desired_extent(width, height)
          .add_image_usage_flags(static_cast<VkImageUsageFlags>(
              vk::ImageUsageFlagBits::eTransferDst))
          .build()
          .value();

  swapchain_extent =
      vk::Extent2D{vkb_swapchain.extent.width, vkb_swapchain.extent.height};
  swapchain = implicit_cast<vk::SwapchainKHR>(vkb_swapchain.swapchain);

  auto c_images = vkb_swapchain.get_images().value();
  swapchain_images.clear();
  swapchain_images.reserve(c_images.size());
  for (auto img : c_images)
    swapchain_images.push_back(implicit_cast<vk::Image>(img));

  auto c_views = vkb_swapchain.get_image_views().value();
  swapchain_image_views.clear();
  swapchain_image_views.reserve(c_views.size());
  for (auto view : c_views)
    swapchain_image_views.push_back(implicit_cast<vk::ImageView>(view));

  LOG_DEBUG_MSG("Swapchain created with {} images", swapchain_images.size());
}

auto VulkanEngine::init_swapchain() -> void {
  create_swapchain(window_extent.width, window_extent.height);

  LOG_INFO_MSG("Swapchain initialized");

  vk::Extent3D draw_img_extent{window_extent.width, window_extent.height, 1};

  draw_img.img_fmt = vk::Format::eR16G16B16A16Sfloat;
  draw_img.img_extent = draw_img_extent;

  vk::ImageUsageFlags draw_img_use_flags =
      vk::ImageUsageFlagBits::eTransferSrc |
      vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage |
      vk::ImageUsageFlagBits::eColorAttachment;

  vk::ImageCreateInfo rimg_info{};
  rimg_info.setImageType(vk::ImageType::e2D);
  rimg_info.setFormat(draw_img.img_fmt);
  rimg_info.setExtent(draw_img_extent);
  rimg_info.setMipLevels(1);
  rimg_info.setArrayLayers(1);
  rimg_info.setSamples(vk::SampleCountFlagBits::e1);
  rimg_info.setTiling(vk::ImageTiling::eOptimal);
  rimg_info.setUsage(draw_img_use_flags);
  rimg_info.setSharingMode(vk::SharingMode::eExclusive);
  rimg_info.setInitialLayout(vk::ImageLayout::eUndefined);

  VmaAllocationCreateInfo rimg_allocinfo = {};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags = static_cast<VkMemoryPropertyFlagBits>(
      vk::MemoryPropertyFlagBits::eDeviceLocal);

  auto c_img_info = implicit_cast<VkImageCreateInfo>(rimg_info);
  VkImage c_image;
  VK_CHECK(static_cast<vk::Result>(
      vmaCreateImage(vma, &c_img_info, &rimg_allocinfo, &c_image,
                     &draw_img.allocation, nullptr)));
  draw_img.img = implicit_cast<vk::Image>(c_image);

  LOG_DEBUG_MSG("Draw image created");

  vk::ImageViewCreateInfo rview_info{};
  rview_info.setImage(draw_img.img);
  rview_info.setViewType(vk::ImageViewType::e2D);
  rview_info.setFormat(draw_img.img_fmt);

  vk::ImageSubresourceRange subresource_range{};
  subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor);
  subresource_range.setBaseMipLevel(0);
  subresource_range.setLevelCount(1);
  subresource_range.setBaseArrayLayer(0);
  subresource_range.setLayerCount(1);
  rview_info.setSubresourceRange(subresource_range);

  draw_img.img_view = device.createImageView(rview_info);

  LOG_DEBUG_MSG("Draw image view created");

  del_queue.push_func([=, this]() {
    device.destroyImageView(draw_img.img_view);
    vmaDestroyImage(vma, implicit_cast<VkImage>(draw_img.img),
                    draw_img.allocation);
  });
}

auto VulkanEngine::destroy_swapchain() -> void {
  LOG_DEBUG_MSG("Destroying swapchain");
  device.destroySwapchainKHR(swapchain);

  for (auto &img_view : swapchain_image_views)
    device.destroyImageView(img_view);

  LOG_DEBUG_MSG("Destroyed {} image views", swapchain_image_views.size());
}

auto VulkanEngine::init_commands() -> void {
  LOG_DEBUG_MSG("Initializing command structures");

  vk::CommandPoolCreateInfo cmd_pool_info{};
  cmd_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  cmd_pool_info.setQueueFamilyIndex(graphics_queue_family);

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    frames[i].cmd_pool = device.createCommandPool(cmd_pool_info);

    vk::CommandBufferAllocateInfo cmd_alloc_info{};
    cmd_alloc_info.setCommandPool(frames[i].cmd_pool);
    cmd_alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    cmd_alloc_info.setCommandBufferCount(1);

    auto buffers = device.allocateCommandBuffers(cmd_alloc_info);
    frames[i].cmd_buf = buffers.front();

    // LOG_TRACE("Created command pool and buffer for frame {}", i);
  }

  imm_cmd_pool = device.createCommandPool(cmd_pool_info);
  vk::CommandBufferAllocateInfo cmd_alloc_info{};
  cmd_alloc_info.setCommandPool(imm_cmd_pool);
  cmd_alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
  cmd_alloc_info.setCommandBufferCount(1);

  auto buffers = device.allocateCommandBuffers(cmd_alloc_info);
  imm_cmd_buf = buffers.front();

  del_queue.push_func([this]() { device.destroyCommandPool(imm_cmd_pool); });

  LOG_INFO_MSG("Commands initialized");
}

auto VulkanEngine::init_sync_structures() -> void {
  LOG_DEBUG_MSG("Initializing synchronization structures");

  vk::FenceCreateInfo fence_create_info{};
  fence_create_info.setFlags(vk::FenceCreateFlagBits::eSignaled);

  vk::SemaphoreCreateInfo semaphore_create_info{};

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    frames[i].render_fence = device.createFence(fence_create_info);
    frames[i].swapchain_semaphore =
        device.createSemaphore(semaphore_create_info);
    frames[i].render_semaphore = device.createSemaphore(semaphore_create_info);

    // LOG_TRACE("Created sync structures for frame {}", i);
  }

  imm_fence = device.createFence(fence_create_info);
  del_queue.push_func([this]() { device.destroyFence(imm_fence); });

  LOG_INFO_MSG("Sync structures initialized");
}

auto VulkanEngine::init_descriptors() -> void {
  LOG_DEBUG_MSG("Initializing descriptor sets");

  std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
      {vk::DescriptorType::eStorageImage, 1.0f}};

  global_desc_allocator.init_pool(device, 10, sizes);
  DescriptorLayoutBuilder buider;
  buider.add_binding(0, vk::DescriptorType::eStorageImage);
  draw_img_desc_set_layout =
      buider.build(device, vk::ShaderStageFlagBits::eCompute);

  draw_img_descriptors =
      global_desc_allocator.allocate(device, draw_img_desc_set_layout);
  vk::DescriptorImageInfo img_info{};
  img_info.setImageLayout(vk::ImageLayout::eGeneral);
  img_info.setImageView(draw_img.img_view);

  vk::WriteDescriptorSet draw_img_write{};
  draw_img_write.setDstBinding(0);
  draw_img_write.setDstSet(draw_img_descriptors);
  draw_img_write.setDescriptorCount(1);
  draw_img_write.setDescriptorType(vk::DescriptorType::eStorageImage);
  draw_img_write.setPImageInfo(&img_info);

  device.updateDescriptorSets(1, &draw_img_write, 0, nullptr);

  del_queue.push_func([&] {
    global_desc_allocator.destroy_pool(device);
    device.destroyDescriptorSetLayout(draw_img_desc_set_layout);
  });

  LOG_DEBUG_MSG("Descriptor sets initialized");
}

auto VulkanEngine::init_pipelines() -> void { init_background_pipelines(); }

auto VulkanEngine::init_background_pipelines() -> void {
  vk::PipelineLayoutCreateInfo info{};
  info.setPSetLayouts(&draw_img_desc_set_layout);
  info.setSetLayoutCount(1);

  gradient_pipeline_layout = device.createPipelineLayout(info);

  auto shader_path =
      (fs::path(ASSET_DIR) / "shaders/gradient.comp.hlsl.spv").string();

  auto compute_draw_shader_opt =
      vkutil::load_shader_module(shader_path, device);

  if (!compute_draw_shader_opt) {
    LOG_ERROR_MSG("Failed to load shader: {}", shader_path);
    return;
  }

  auto compute_draw_shader = compute_draw_shader_opt.value();

  if (!compute_draw_shader_opt) {
    LOG_ERROR_MSG("Error when building the compute shader.");
    return;
  }

  vk::PipelineShaderStageCreateInfo stage_info{};
  stage_info.setStage(vk::ShaderStageFlagBits::eCompute);
  stage_info.setModule(compute_draw_shader);
  stage_info.setPName("main");

  vk::ComputePipelineCreateInfo create_info{};
  create_info.setLayout(gradient_pipeline_layout);
  create_info.setStage(stage_info);

  auto res = device.createComputePipeline(vk::PipelineCache{}, create_info);
  VK_CHECK(res.result);
  gradient_pipeline = res.value;

  device.destroyShaderModule(compute_draw_shader);

  del_queue.push_func([&] {
    device.destroyPipelineLayout(gradient_pipeline_layout);
    device.destroyPipeline(gradient_pipeline);
  });
}

auto VulkanEngine::imm_submit(std::function<void(vk::CommandBuffer)> &&func)
    -> void {
  device.resetFences(imm_fence);
  imm_cmd_buf.reset();

  auto cmd = imm_cmd_buf;
  vk::CommandBufferBeginInfo cmd_buf_beg_info{};
  cmd_buf_beg_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  cmd.begin(cmd_buf_beg_info);
  func(cmd);
  cmd.end();

  vk::CommandBufferSubmitInfo cmd_buf_sub_info{};
  cmd_buf_sub_info.setCommandBuffer(cmd);
  cmd_buf_sub_info.setDeviceMask(0);

  vk::SubmitInfo2 sub_info{};
  sub_info.setCommandBufferInfoCount(1);
  sub_info.setPCommandBufferInfos(&cmd_buf_sub_info);

  graphics_queue.submit2(sub_info, imm_fence);

  VK_CHECK(device.waitForFences(imm_fence, true, 1'000'000'000U));
}

auto VulkanEngine::init_imgui() -> void {
  LOG_DEBUG_MSG("Initializing ImGui");
  // 1: create descriptor pool for IMGUI
  //  the size of the pool is very oversize, but it's copied from imgui demo
  //  itself.
  std::array<vk::DescriptorPoolSize, 11> pool_sizes = {{
      {vk::DescriptorType::eSampler, 1000U},
      {vk::DescriptorType::eCombinedImageSampler, 1000U},
      {vk::DescriptorType::eSampledImage, 1000U},
      {vk::DescriptorType::eStorageImage, 1000U},
      {vk::DescriptorType::eUniformTexelBuffer, 1000U},
      {vk::DescriptorType::eStorageTexelBuffer, 1000U},
      {vk::DescriptorType::eUniformBuffer, 1000U},
      {vk::DescriptorType::eStorageBuffer, 1000U},
      {vk::DescriptorType::eUniformBufferDynamic, 1000U},
      {vk::DescriptorType::eStorageBufferDynamic, 1000U},
      {vk::DescriptorType::eInputAttachment, 1000U},
  }};

  vk::DescriptorPoolCreateInfo pool_info{};
  pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
  pool_info.setMaxSets(1000U);
  pool_info.setPoolSizeCount(pool_sizes.size());
  pool_info.setPPoolSizes(pool_sizes.data());

  imgui_pool = device.createDescriptorPool(pool_info);

  // 2: initialize imgui library

  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.FontGlobalScale = 0.8f;

  ImGui_ImplGlfw_InitForVulkan(window, 1);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = instance;
  init_info.PhysicalDevice = gpu;
  init_info.Device = device;
  init_info.Queue = graphics_queue;
  init_info.DescriptorPool = imgui_pool;
  init_info.MinImageCount = 3;
  init_info.ImageCount = 3;
  init_info.UseDynamicRendering = true;

  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount =
      1;
  // i hate doing reinterpret_cast but there is nothing we can do because ImGui
  // accepts only C API
  VkFormat imgui_format = static_cast<VkFormat>(swapchain_img_fmt);
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo
      .pColorAttachmentFormats = &imgui_format;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo
      .pColorAttachmentFormats = &imgui_format;

  init_info.PipelineInfoMain.MSAASamples =
      static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1);

  ImGui_ImplVulkan_Init(&init_info);

  // ImGui_ImplVulkan_CreateFontsTexture();

  del_queue.push_func([&]() {
    ImGui_ImplVulkan_Shutdown();
    device.destroyDescriptorPool(imgui_pool);
  });

  LOG_DEBUG_MSG("ImGui Initialized");
}

auto VulkanEngine::draw_background(vk::CommandBuffer cmd) -> void {
  // LOG_TRACE("Drawing ImGui");

  cmd.bindPipeline(vk::PipelineBindPoint::eCompute, gradient_pipeline);
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                         gradient_pipeline_layout, 0, 1, &draw_img_descriptors,
                         0, nullptr);

  // execute the compute pipeline dispatch. We are using 16x16 workgroup
  // size so we need to divide by it
  cmd.dispatch(std::ceil(draw_extent.width / 16.0),
               std::ceil(draw_extent.height / 16.0), 1);
}

auto VulkanEngine::draw_imgui(vk::CommandBuffer cmd,
                              vk::ImageView target_img_view) -> void {
  vk::RenderingAttachmentInfo colour_attachment{};
  colour_attachment.setImageView(target_img_view);
  colour_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
  colour_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
  colour_attachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);

  vk::RenderingInfo render_info{};
  render_info.setRenderArea(vk::Rect2D{vk::Offset2D{0, 0}, swapchain_extent});
  render_info.setLayerCount(1);
  render_info.setColorAttachmentCount(1);
  render_info.setPColorAttachments(&colour_attachment);
  render_info.setPStencilAttachment(nullptr);
  render_info.setPDepthAttachment(nullptr);
  render_info.setPStencilAttachment(nullptr);

  cmd.beginRendering(render_info);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  implicit_cast<VkCommandBuffer>(cmd));

  cmd.endRendering();
}

auto VulkanEngine::draw() -> void {
  // LOG_TRACE("Drawing frame {}", frame_number);

  auto wait_result = device.waitForFences(get_current_frame().render_fence,
                                          true, 1'000'000'000U);
  VK_CHECK(wait_result);

  get_current_frame().del_queue.flush();

  device.resetFences(get_current_frame().render_fence);

  auto [result, swapchain_image_idx] = device.acquireNextImageKHR(
      swapchain, 1'000'000'000U, get_current_frame().swapchain_semaphore);
  VK_CHECK(result);

  // LOG_TRACE("Acquired swapchain image {}", swapchain_image_idx);

  vk::CommandBuffer cmd = get_current_frame().cmd_buf;
  cmd.reset();

  vk::CommandBufferBeginInfo cmd_buf_beg_info{};
  cmd_buf_beg_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  draw_extent.width = swapchain_extent.width;
  draw_extent.height = swapchain_extent.height;

  cmd.begin(cmd_buf_beg_info);

  vkutil::transition_image(cmd, draw_img.img, vk::ImageLayout::eUndefined,
                           vk::ImageLayout::eGeneral);

  draw_background(cmd);

  vkutil::transition_image(cmd, draw_img.img, vk::ImageLayout::eGeneral,
                           vk::ImageLayout::eTransferSrcOptimal);
  vkutil::transition_image(cmd, swapchain_images[swapchain_image_idx],
                           vk::ImageLayout::eUndefined,
                           vk::ImageLayout::eTransferDstOptimal);

  vkutil::copy_image_to_image(cmd, draw_img.img,
                              swapchain_images[swapchain_image_idx],
                              draw_extent, swapchain_extent);

  vkutil::transition_image(cmd, swapchain_images[swapchain_image_idx],
                           vk::ImageLayout::eTransferDstOptimal,
                           vk::ImageLayout::eColorAttachmentOptimal);

  draw_imgui(cmd, swapchain_image_views[swapchain_image_idx]);

  vkutil::transition_image(cmd, swapchain_images[swapchain_image_idx],
                           vk::ImageLayout::eColorAttachmentOptimal,
                           vk::ImageLayout::ePresentSrcKHR);

  cmd.end();

  vk::CommandBufferSubmitInfo cmd_buf_sub_info{};
  cmd_buf_sub_info.setCommandBuffer(cmd);
  cmd_buf_sub_info.setDeviceMask(0);

  vk::SemaphoreSubmitInfo wait_info{};
  wait_info.setSemaphore(get_current_frame().swapchain_semaphore);
  wait_info.setValue(1);
  wait_info.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
  wait_info.setDeviceIndex(0);

  vk::SemaphoreSubmitInfo signal_info{};
  signal_info.setSemaphore(get_current_frame().render_semaphore);
  signal_info.setValue(1);
  signal_info.setStageMask(vk::PipelineStageFlagBits2::eAllGraphics);
  signal_info.setDeviceIndex(0);

  vk::SubmitInfo2 sub_info{};
  sub_info.setWaitSemaphoreInfoCount(1);
  sub_info.setPWaitSemaphoreInfos(&wait_info);
  sub_info.setCommandBufferInfoCount(1);
  sub_info.setPCommandBufferInfos(&cmd_buf_sub_info);
  sub_info.setSignalSemaphoreInfoCount(1);
  sub_info.setPSignalSemaphoreInfos(&signal_info);

  graphics_queue.submit2(sub_info, get_current_frame().render_fence);

  vk::PresentInfoKHR present_info{};
  present_info.setWaitSemaphoreCount(1);
  present_info.setPWaitSemaphores(&get_current_frame().render_semaphore);
  present_info.setSwapchainCount(1);
  present_info.setPSwapchains(&swapchain);
  present_info.setPImageIndices(&swapchain_image_idx);

  VK_CHECK(graphics_queue.presentKHR(present_info));

  frame_number++;
}

auto VulkanEngine::cleanup() -> void {
  LOG_INFO_MSG("Cleaning up Vulkan Engine");
  if (!is_initialized) {
    LOG_WARNING_MSG("::cleanup() called with uninitialized instance!");
    return;
  }

  device.waitIdle();

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    device.destroyCommandPool(frames[i].cmd_pool);
    device.destroyFence(frames[i].render_fence);
    device.destroySemaphore(frames[i].render_semaphore);
    device.destroySemaphore(frames[i].swapchain_semaphore);

    frames[i].del_queue.flush();
  }
  LOG_DEBUG_MSG("Frame resources destroyed");

  del_queue.flush();

  destroy_swapchain();
  LOG_DEBUG_MSG("Swapchain destroyed");

  instance.destroySurfaceKHR(surface);
  LOG_DEBUG_MSG("Surface destroyed");

  device.destroy();
  LOG_DEBUG_MSG("Logical device destroyed");

  vkb::destroy_debug_utils_messenger(
      implicit_cast<VkInstance>(instance),
      implicit_cast<VkDebugUtilsMessengerEXT>(dbg_msngr));
  LOG_DEBUG_MSG("Debug utils messenger destroyed");

  instance.destroy();
  LOG_DEBUG_MSG("VkInstance destroyed");

  glfwDestroyWindow(window);
  LOG_DEBUG_MSG("GLFWWindow destroyed");
  glfwTerminate();

  LOG_INFO_MSG("Vulkan Engine cleaned up successfully");
}
