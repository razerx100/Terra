#ifndef DESCRIPTOR_SET_MANAGER_HPP_
#define DESCRIPTOR_SET_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DescriptorPool.hpp>
#include <memory>

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
	VkDescriptorSetLayout GetDescriptorSetLayout() const noexcept;
	[[nodiscard]]
	VkDescriptorSet GetDescriptorSet() const noexcept;

	void AddSetLayoutAndQueueForBinding(
		DescriptorInfo descInfo, VkShaderStageFlags shaderFlag,
		std::vector<VkDescriptorBufferInfo>&& bufferInfo
	);

	void AddSetLayoutAndQueueForBinding(
		DescriptorInfo descInfo, VkShaderStageFlags shaderFlag,
		std::vector<VkDescriptorImageInfo>&& imageInfo
	);

	void AddSetLayout(
		const DescriptorInfo& descInfo, VkShaderStageFlags shaderFlag
	);

	void CreateDescriptorSets(VkDevice device);

private:
	void CreateSetLayout(VkDevice device);

private:
	VkDevice m_deviceRef;
	VkDescriptorSet m_descriptorSet;
	VkDescriptorSetLayout m_descriptorSetLayout;
	std::unique_ptr<DescriptorPool> m_descriptorPool;
	std::vector<DescBufferInfo> m_bufferInfos;
	std::vector<DescImageInfo> m_imageInfos;
	std::vector<VkDescriptorSetLayoutBinding> m_layoutBindings;
};
#endif
