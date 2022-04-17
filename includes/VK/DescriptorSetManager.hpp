#ifndef DESCRIPTOR_SET_MANAGER_HPP_
#define DESCRIPTOR_SET_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DescriptorPool.hpp>
#include <memory>

using VulkanDescriptorSetLayouts = const std::vector<VkDescriptorSetLayout>&;

struct BindBufferInputInfo {
	VkDevice device;
	VkBuffer buffer;
	std::uint32_t bufferOffset;
	std::uint32_t bufferSize;
	std::uint32_t bindingSlot;
	std::uint32_t descriptorCount;
	VkDescriptorType type;
};

class DescriptorSetManager {
public:
	DescriptorSetManager(VkDevice device);
	~DescriptorSetManager() noexcept;

	[[nodiscard]]
	VulkanDescriptorSetLayouts GetDescriptorSetLayouts() const noexcept;
	[[nodiscard]]
	VkDescriptorSet GetDescriptorSet() const noexcept;

	void AddSetLayout(
		VkDevice device,
		VkDescriptorType type, VkShaderStageFlags shaderBits,
		std::uint32_t bindingSlot, std::uint32_t descriptorCount
	);

	void CreateDescriptorSets(VkDevice device);
	void BindBuffer(
		const BindBufferInputInfo& inputInfo
	);

private:
	VkDevice m_deviceRef;
	VkDescriptorSet m_descriptorSet;
	std::unique_ptr<DescriptorPool> m_descriptorPool;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
};
#endif
