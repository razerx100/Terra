#ifndef DESCRIPTOR_POOL_HPP_
#define DESCRIPTOR_POOL_HPP_
#include <vulkan/vulkan.hpp>
#include <unordered_map>

class DescriptorPool {
public:
	DescriptorPool(VkDevice device) noexcept;
	~DescriptorPool() noexcept;

	void AddDescriptorTypeLimit(
		VkDescriptorType type, std::uint32_t descriptorCount
	) noexcept;
	void CreatePool(VkDevice device, std::uint32_t setLayoutCount);
	void AllocateDescriptors(
		VkDevice device, const VkDescriptorSetLayout* setLayouts,
		VkDescriptorSet* descriptorSets, std::uint32_t setCount = 1u
	) const;

private:
	VkDevice m_deviceRef;
	VkDescriptorPool m_descriptorPool;
	std::unordered_map<VkDescriptorType, std::uint32_t> m_typeMap;
};
#endif
