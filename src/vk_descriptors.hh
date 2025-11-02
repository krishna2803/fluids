#pragma once

#include "vk_types.hh"

struct DescriptorLayoutBuilder {
  std::vector<vk::DescriptorSetLayoutBinding> bindings;

  void add_binding(u32 binding, vk::DescriptorType type);
  void clear();

  vk::DescriptorSetLayout build(vk::Device &device,
                                vk::ShaderStageFlags shader_stages,
                                vk::DescriptorSetLayoutCreateFlags flags = {});
};

struct DescriptorAllocator {

  struct PoolSizeRatio {
    vk::DescriptorType type;
    float ratio;
  };

  vk::DescriptorPool pool;

  void init_pool(vk::Device &device, u32 max_sets,
                 std::span<PoolSizeRatio> pool_ratios);
  void clear_descriptors(vk::Device &device);
  void destroy_pool(vk::Device &device);

  vk::DescriptorSet allocate(vk::Device &device,
                             vk::DescriptorSetLayout layout);
};
