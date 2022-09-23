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

class DescriptorSetManager {
public:
	DescriptorSetManager(VkDevice device, size_t bufferCount);
	~DescriptorSetManager() noexcept;

	[[nodiscard]]
	VkDescriptorSetLayout GetDescriptorSetLayout() const noexcept;
	[[nodiscard]]
	VkDescriptorSet GetDescriptorSet(size_t index) const noexcept;

	void AddSetLayout(
		const DescriptorInfo& descInfo, VkShaderStageFlagBits shaderFlag,
		std::vector<VkDescriptorBufferInfo>&& bufferInfo
	) noexcept;

	void AddSetLayout(
		const DescriptorInfo& descInfo, VkShaderStageFlagBits shaderFlag,
		std::vector<VkDescriptorImageInfo>&& imageInfo
	) noexcept;

	void CreateDescriptorSets(VkDevice device);

private:
	void CreateSetLayout(VkDevice device);
	void _addSetLayout(const DescriptorInfo& descInfo, VkShaderStageFlagBits shaderFlag);

	static void BindBuffer(
		VkDevice device, VkDescriptorSet descSet,
		const DescriptorInfo& descriptorInfo, const VkDescriptorBufferInfo& bufferInfo
	) noexcept;
	static void BindImageView(
		VkDevice device, VkDescriptorSet descSet,
		const DescImageInfo& imageInfo
	) noexcept;

private:
	VkDevice m_deviceRef;
	std::vector<VkDescriptorSet> m_descriptorSets;
	VkDescriptorSetLayout m_descriptorSetLayout;
	DescriptorPool m_descriptorPool;
	std::vector<DescBufferInfo> m_bufferInfos;
	std::vector<DescImageInfo> m_imageInfos;
	std::vector<VkDescriptorSetLayoutBinding> m_layoutBindings;
	std::vector<VkDescriptorBindingFlags> m_bindingFlags;
};
#endif
