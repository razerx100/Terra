#ifndef DESCRIPTOR_SET_MANAGER_HPP_
#define DESCRIPTOR_SET_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DescriptorPool.hpp>
#include <memory>

struct DescriptorInfo {
	std::uint32_t bindingSlot;
	std::uint32_t descriptorCount = 1u;
	VkDescriptorType type;
};

class DescriptorSetManager {
	class DescriptorInstance {
	public:
		virtual ~DescriptorInstance() = default;

		void UpdateDescriptors(
			VkDevice device, const std::vector<VkDescriptorSet>& descSets
		) const noexcept;

	protected:
		[[nodiscard]]
		virtual std::vector<VkWriteDescriptorSet> PopulateWriteDescSets(
			const std::vector<VkDescriptorSet>& descSets
		) const noexcept = 0;
	};

public:
	DescriptorSetManager(VkDevice device, size_t bufferCount);
	~DescriptorSetManager() noexcept;

	[[nodiscard]]
	VkDescriptorSetLayout const* GetDescriptorSetLayouts() const noexcept;
	[[nodiscard]]
	VkDescriptorSet GetDescriptorSet(size_t index) const noexcept;

	void AddBuffersSplit(
		const DescriptorInfo& descInfo, std::vector<VkDescriptorBufferInfo> bufferInfos,
		VkShaderStageFlagBits shaderFlag
	) noexcept;
	void AddBuffersContiguous(
		const DescriptorInfo& descInfo, std::vector<VkDescriptorBufferInfo> bufferInfos,
		VkShaderStageFlagBits shaderFlag
	) noexcept;
	void AddImagesSplit(
		const DescriptorInfo& descInfo, std::vector<VkDescriptorImageInfo> imageInfos,
		VkShaderStageFlagBits shaderFlag
	) noexcept;
	void AddImagesContiguous(
		const DescriptorInfo& descInfo, std::vector<VkDescriptorImageInfo> imageInfos,
		VkShaderStageFlagBits shaderFlag
	) noexcept;

	void CreateDescriptorSets(VkDevice device);

private:
	void CreateSetLayouts(VkDevice device);
	void AddSetLayout(const DescriptorInfo& descInfo, VkShaderStageFlagBits shaderFlag);

private:
	VkDevice m_deviceRef;
	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	DescriptorPool m_descriptorPool;
	std::vector<std::unique_ptr<DescriptorInstance>> m_descriptorInstances;
	std::vector<VkDescriptorSetLayoutBinding> m_layoutBindings;
	std::vector<VkDescriptorBindingFlags> m_bindingFlags;

private:
	class DescriptorInstanceMembers : public DescriptorInstance {
	public:
		DescriptorInstanceMembers() noexcept : m_descriptorInfo{}, m_isSplit{ false } {}

	protected:
		DescriptorInfo m_descriptorInfo;
		bool m_isSplit;
	};

	class DescriptorInstanceBuffer : public DescriptorInstanceMembers {
	public:
		void AddBuffersSplit(
			const DescriptorInfo& descInfo, std::vector<VkDescriptorBufferInfo> bufferInfos
		) noexcept;
		void AddBuffersContiguous(
			const DescriptorInfo& descInfo, std::vector<VkDescriptorBufferInfo> bufferInfos
		) noexcept;

	private:
		[[nodiscard]]
		std::vector<VkWriteDescriptorSet> PopulateWriteDescSets(
			const std::vector<VkDescriptorSet>& descSets
		) const noexcept override;

	private:
		std::vector<VkDescriptorBufferInfo> m_bufferInfos;
	};

	class DescriptorInstanceImage : public DescriptorInstanceMembers {
	public:
		void AddImagesSplit(
			const DescriptorInfo& descInfo, std::vector<VkDescriptorImageInfo> imageInfos
		) noexcept;
		void AddImagesContiguous(
			const DescriptorInfo& descInfo, std::vector<VkDescriptorImageInfo> imageInfos
		) noexcept;

	private:
		[[nodiscard]]
		std::vector<VkWriteDescriptorSet> PopulateWriteDescSets(
			const std::vector<VkDescriptorSet>& descSets
		) const noexcept override;

	private:
		std::vector<VkDescriptorImageInfo> m_imageInfos;
	};
};

#endif
