#include "vk_descriptors.hh"
#include "vulkan/vulkan.hpp"

auto DescriptorLayoutBuilder::add_binding(u32 binding, vk::DescriptorType type)
    -> void {
  vk::DescriptorSetLayoutBinding new_binding{};
  new_binding.setBinding(binding);
  new_binding.setDescriptorCount(1);
  new_binding.setDescriptorType(type);

  bindings.emplace_back(new_binding);
}

auto DescriptorLayoutBuilder::clear() -> void { bindings.clear(); }

vk::DescriptorSetLayout
DescriptorLayoutBuilder::build(vk::Device &device,
                               vk::ShaderStageFlags shader_stages,
                               vk::DescriptorSetLayoutCreateFlags flags) {
  for (auto &b : bindings)
    b.stageFlags |= shader_stages;

  vk::DescriptorSetLayoutCreateInfo info{};
  info.setPBindings(bindings.data());
  info.setBindingCount(bindings.size());
  info.setFlags(flags);

  return device.createDescriptorSetLayout(info);
}

auto DescriptorAllocator::init_pool(vk::Device &device, u32 max_sets,
                                    std::span<PoolSizeRatio> pool_ratios)
    -> void {
  std::vector<vk::DescriptorPoolSize> pool_sizes;
  for (auto ratio : pool_ratios) {
    vk::DescriptorPoolSize sz{};
    sz.type = ratio.type;
    sz.descriptorCount = implicit_cast<u32>(ratio.ratio * max_sets);
    pool_sizes.emplace_back(sz);
  }

  vk::DescriptorPoolCreateInfo pool_info{};
  pool_info.setFlags(vk::DescriptorPoolCreateFlagBits{});
  pool_info.setMaxSets(max_sets);
  pool_info.setPoolSizeCount(pool_sizes.size());
  pool_info.setPPoolSizes(pool_sizes.data());

  pool = device.createDescriptorPool(pool_info);
}

auto DescriptorAllocator::clear_descriptors(vk::Device &device) -> void {
  device.resetDescriptorPool(pool, vk::DescriptorPoolResetFlags{});
}

auto DescriptorAllocator::destroy_pool(vk::Device &device) -> void {
  device.destroyDescriptorPool(pool);
}

auto DescriptorAllocator::allocate(vk::Device &device,
                                   vk::DescriptorSetLayout layout)
    -> vk::DescriptorSet {
  vk::DescriptorSetAllocateInfo info{};
  info.setDescriptorPool(pool);
  info.setDescriptorSetCount(1);
  info.setPSetLayouts(&layout);

  auto ds = device.allocateDescriptorSets(info);

  if (ds.size() != 1) [[unlikely]]
    std::unreachable();

  return ds.front();
}
