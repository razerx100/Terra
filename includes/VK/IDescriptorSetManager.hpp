#ifndef __I_DESCRIPTOR_SET_MANAGER_HPP__
#define __I_DESCRIPTOR_SET_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>

using VulkanDescriptorSetLayouts = const std::vector<VkDescriptorSetLayout>&;

class IDescriptorSetManager {
public:
	virtual ~IDescriptorSetManager() = default;

	virtual VulkanDescriptorSetLayouts GetDescriptorSetLayouts() const noexcept
		= 0;
	virtual VkDescriptorSet GetDescriptorSet() const noexcept = 0;

	virtual void AddSetLayout(
		VkDevice device,
		VkDescriptorType type, VkShaderStageFlags shaderBits,
		std::uint32_t bindingSlot, std::uint32_t descriptorCount
	) = 0;

	virtual void CreateDescriptorSets(VkDevice device) = 0;
	virtual void BindBuffer(
		VkDevice device, VkBuffer buffer,
		std::uint32_t bufferOffset, std::uint32_t bufferSize,
		std::uint32_t bindingSlot, std::uint32_t descriptorCount,
		VkDescriptorType descType
	) = 0;
};

IDescriptorSetManager* CreateDescriptorSetManagerInstance(VkDevice device);
#endif
