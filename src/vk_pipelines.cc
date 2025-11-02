#include "vk_pipelines.hh"
#include "logger.hh"
#include "vk_types.hh"

#include <cstddef>
#include <fstream>

namespace vkutil {
std::optional<vk::ShaderModule> load_shader_module(std::string_view filename,
                                                   vk::Device &device) {
  // seek to the end of stream immediately after opening in binary mode
  LOG_DEBUG_MSG("Loading SPIR-V shader file `{}`", filename);

  std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);

  if (!file.is_open())
    return std::nullopt;

  std::size_t file_size = file.tellg();

  if (file_size % sizeof(u32) != 0)
    LOG_WARNING_MSG(
        "filesize of `{}` not 4-byte aligned. is ts correct spir-v file?",
        filename);

  std::vector<u32> buffer((file_size + sizeof(u32) - 1) / sizeof(u32));

  file.seekg(0);

  // load the entire file into the buffer
  // not UB because ts allowed for some reason
  file.read(reinterpret_cast<char *>(buffer.data()), file_size);

  vk::ShaderModuleCreateInfo info{};
  info.setCodeSize(buffer.size() * sizeof(u32));
  info.setPCode(buffer.data());

  return device.createShaderModule(info);
}
}; // namespace vkutil
