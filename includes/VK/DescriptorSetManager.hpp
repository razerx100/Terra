#ifndef DESCRIPTOR_SET_MANAGER_HPP_
#define DESCRIPTOR_SET_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DescriptorPool.hpp>
#include <memory>

class DescriptorSetManager {
public:
	using VulkanDescriptorSetLayouts = const std::vector<VkDescriptorSetLayout>&;

	struct DescriptorInfo {
		std::uint32_t bindingSlot;
		std::uint32_t descriptorCount;
		VkDescriptorType type;
	};

	struct BufferInfo {
		DescriptorInfo descriptorInfo;
		std::vector<VkDescriptorBufferInfo> buffers;
	};

	struct ImageInfo {
		DescriptorInfo descriptorInfo;
		std::vector<VkDescriptorImageInfo> images;
	};

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
