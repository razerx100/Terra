#ifndef __I_DESCRIPTOR_POOL_HPP__
#define __I_DESCRIPTOR_POOL_HPP__
#include <vulkan/vulkan.hpp>

enum class DescriptorType {
	UniformBuffer,
	StorageBuffer,
	UniformBufferDynamic
};

class IDescriptorPool {
public:
	virtual ~IDescriptorPool() = default;

	virtual void AddDescriptorTypeLimit(
		DescriptorType type, std::uint32_t descriptorCount
	) noexcept = 0;
	virtual void CreatePool(VkDevice device, std::uint32_t setLayoutCount) = 0;
	virtual void AllocateDescriptors(
		VkDevice device, const VkDescriptorSetLayout* setLayouts,
		VkDescriptorSet* descriptorSets, std::uint32_t setCount = 1u
	) const = 0;
};
#endif
