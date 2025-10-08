#include "logger.hh"
#include "vk_engine.hh"

int main() {
  Logger::Get().init();

  LOG_INFO_MSG("Starting Vulkan Engine");

  VulkanEngine::Get().init();
  VulkanEngine::Get().run();
  VulkanEngine::Get().cleanup();

  LOG_INFO_MSG("Stopping Vulkan Engine");

  return 0;
}