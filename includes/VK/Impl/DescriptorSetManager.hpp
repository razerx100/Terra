#ifndef __DESCRIPTOR_SET_MANAGER_HPP__
#define __DESCRIPTOR_SET_MANAGER_HPP__
#include <IDescriptorSetManager.hpp>
#include <DescriptorPool.hpp>
#include <memory>

class DescriptorSetManager : public IDescriptorSetManager {
public:
	DescriptorSetManager(VkDevice device);
	~DescriptorSetManager() noexcept override;

	[[nodiscard]]
	VulkanDescriptorSetLayouts GetDescriptorSetLayouts() const noexcept override;
	[[nodiscard]]
	VkDescriptorSet GetDescriptorSet() const noexcept override;

	void AddSetLayout(
		VkDevice device,
		VkDescriptorType type, VkShaderStageFlags shaderBits,
		std::uint32_t bindingSlot, std::uint32_t descriptorCount
	) override;

	void CreateDescriptorSets(VkDevice device) override;
	void BindBuffer(
		VkDevice device, VkBuffer buffer,
		std::uint32_t bufferOffset, std::uint32_t bufferSize,
		std::uint32_t bindingSlot, std::uint32_t descriptorCount,
		VkDescriptorType descType
	) override;

private:
	VkDevice m_deviceRef;
	VkDescriptorSet m_descriptorSet;
	std::unique_ptr<IDescriptorPool> m_descriptorPool;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
};
#endif
