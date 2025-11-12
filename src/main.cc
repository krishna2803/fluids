#include "logger.hh"
#include "vk_engine.hh"

int main() {
  Logger::Get().init();
  VulkanEngine::Get().init();
  VulkanEngine::Get().run();
  VulkanEngine::Get().cleanup();
  return 0;
}
