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

struct BindBufferInputInfo {
	VkDevice device;
	VkBuffer buffer;
	std::uint32_t bufferSize;
	VkShaderStageFlags shaderBits;
	DescriptorInfo descriptorInfo;
};

struct BindImageViewInputInfo {
	VkDevice device;
	VkImageView imageView;
	VkSampler sampler;
	VkShaderStageFlags shaderBits;
	DescriptorInfo descriptorInfo;
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
		const BindBufferInputInfo& inputInfo
	);

	void AddSetLayout(
		const BindImageViewInputInfo& inputInfo
	);

	void CreateDescriptorSets(VkDevice device);

private:
	struct BufferInfo {
		VkBuffer buffer;
		VkDeviceSize size;
		DescriptorInfo descriptorInfo;
	};

	struct ImageInfo {
		VkImageView imageView;
		VkSampler sampler;
		DescriptorInfo descriptorInfo;
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
