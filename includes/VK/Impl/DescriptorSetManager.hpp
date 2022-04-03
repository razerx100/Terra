#ifndef __DESCRIPTOR_SET_MANAGER_HPP__
#define __DESCRIPTOR_SET_MANAGER_HPP__
#include <IDescriptorSetManager.hpp>
#include <DescriptorPool.hpp>
#include <memory>

class DescriptorSetManager : public IDescriptorSetManager {
public:
	DescriptorSetManager(VkDevice device);
	~DescriptorSetManager() noexcept;

	VulkanDescriptorSetLayouts GetDescriptorSetLayouts() const noexcept override;
	VkDescriptorSet GetDescriptorSet() const noexcept override;

	void AddSetLayout(
		VkDevice device,
		VkDescriptorType type, VkShaderStageFlags shaderBits,
		std::uint32_t bindingSlot, std::uint32_t descriptorCount
	) override;

	void CreateDescriptorSets(VkDevice device) override;

private:
	VkDevice m_deviceRef;
	VkDescriptorSet m_descriptorSet;
	std::unique_ptr<IDescriptorPool> m_descriptorPool;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
};
#endif
