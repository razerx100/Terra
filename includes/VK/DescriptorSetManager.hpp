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

class DescriptorSetManager {
public:
	DescriptorSetManager(VkDevice device);
	~DescriptorSetManager() noexcept;

	[[nodiscard]]
	VulkanDescriptorSetLayouts GetDescriptorSetLayouts() const noexcept;
	[[nodiscard]]
	VkDescriptorSet GetDescriptorSet() const noexcept;

	void AddSetLayout(
		VkDevice device, DescriptorInfo descInfo,
		VkShaderStageFlags shaderFlag,
		std::vector<VkDescriptorBufferInfo>&& bufferInfo
	);

	void AddSetLayout(
		VkDevice device, DescriptorInfo descInfo,
		VkShaderStageFlags shaderFlag,
		std::vector<VkDescriptorImageInfo>&& imageInfo
	);

	void CreateDescriptorSets(VkDevice device);

private:
	struct BufferInfo {
		DescriptorInfo descriptorInfo;
		std::vector<VkDescriptorBufferInfo> buffers;
	};

	struct ImageInfo {
		DescriptorInfo descriptorInfo;
		std::vector<VkDescriptorImageInfo> images;
	};

	void BindBuffer(
		VkDevice device, const BufferInfo& bufferInfo
	) const noexcept;

	void BindImageView(
		VkDevice device, const ImageInfo& imageInfo
	) const noexcept;


private:
	VkDevice m_deviceRef;
	VkDescriptorSet m_descriptorSet;
	std::unique_ptr<DescriptorPool> m_descriptorPool;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	std::vector<BufferInfo> m_bufferInfos;
	std::vector<ImageInfo> m_imageInfos;
};
#endif
