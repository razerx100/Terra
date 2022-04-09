#ifndef __DESCRIPTOR_POOL_HPP__
#define __DESCRIPTOR_POOL_HPP__
#include <IDescriptorPool.hpp>
#include <vector>

class DescriptorPool : public IDescriptorPool {
public:
	DescriptorPool(VkDevice device) noexcept;
	~DescriptorPool() noexcept override;

	void AddDescriptorTypeLimit(
		DescriptorType type, std::uint32_t descriptorCount
	) noexcept override;
	void CreatePool(VkDevice device, std::uint32_t setLayoutCount) override;
	void AllocateDescriptors(
		VkDevice device, const VkDescriptorSetLayout* setLayouts,
		VkDescriptorSet* descriptorSets, std::uint32_t setCount = 1u
	) const override;

private:
	VkDevice m_deviceRef;
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorPoolSize> m_descriptorTypeCounts;
	std::vector<std::int32_t> m_typeMap;
};
#endif
