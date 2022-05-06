#ifndef DESCRIPTOR_SET_MANAGER_HPP_
#define DESCRIPTOR_SET_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DescriptorPool.hpp>
#include <memory>

using VulkanDescriptorSetLayouts = const std::vector<VkDescriptorSetLayout>&;

struct DescriptorInfo {
	std::uint32_t bindingSlot;
	std::uint32_t descriptorCount;
	VkDescriptorType type;
};

struct DescBufferInfo {
	DescriptorInfo descriptorInfo;
	std::vector<VkDescriptorBufferInfo> buffers;
};

struct DescImageInfo {
	DescriptorInfo descriptorInfo;
	std::vector<VkDescriptorImageInfo> images;
};

void BindBuffer(
	VkDevice device, VkDescriptorSet descSet,
	const DescBufferInfo& bufferInfo
) noexcept;

void BindImageView(
	VkDevice device, VkDescriptorSet descSet,
	const DescImageInfo& imageInfo
) noexcept;

class DescriptorSetManager {
public:
	DescriptorSetManager(VkDevice device);
	~DescriptorSetManager() noexcept;

	[[nodiscard]]
	VulkanDescriptorSetLayouts GetDescriptorSetLayouts() const noexcept;
	[[nodiscard]]
	VkDescriptorSet GetDescriptorSet() const noexcept;

	void AddSetLayoutAndQueueForBinding(
		VkDevice device, DescriptorInfo descInfo,
		VkShaderStageFlags shaderFlag,
		std::vector<VkDescriptorBufferInfo>&& bufferInfo
	);

	void AddSetLayoutAndQueueForBinding(
		VkDevice device, DescriptorInfo descInfo,
		VkShaderStageFlags shaderFlag,
		std::vector<VkDescriptorImageInfo>&& imageInfo
	);

	void AddSetLayoutImage(
		VkDevice device, const DescriptorInfo& descInfo,
		VkShaderStageFlags shaderFlag
	);

	void AddSetLayoutBuffer(
		VkDevice device, const DescriptorInfo& descInfo,
		VkShaderStageFlags shaderFlag
	);

	void CreateDescriptorSets(VkDevice device);

private:
	VkDevice m_deviceRef;
	VkDescriptorSet m_descriptorSet;
	std::unique_ptr<DescriptorPool> m_descriptorPool;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<DescBufferInfo> m_bufferInfos;
	std::vector<DescImageInfo> m_imageInfos;
};
#endif
