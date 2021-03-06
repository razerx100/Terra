#include <DescriptorPool.hpp>
#include <VKThrowMacros.hpp>
#include <vector>

DescriptorPool::DescriptorPool(VkDevice device) noexcept
	: m_deviceRef(device), m_descriptorPool(VK_NULL_HANDLE) {}

DescriptorPool::~DescriptorPool() noexcept {
	vkDestroyDescriptorPool(m_deviceRef, m_descriptorPool, nullptr);
}

void DescriptorPool::AddDescriptorTypeLimit(
	VkDescriptorType type, std::uint32_t descriptorCount
) noexcept {
	m_typeMap[type] += descriptorCount;
}

void DescriptorPool::CreatePool(VkDevice device) {
	std::vector<VkDescriptorPoolSize> descriptorTypeCounts;
	for (auto& [type, descCount] : m_typeMap)
		descriptorTypeCounts.emplace_back(type, descCount);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	poolInfo.maxSets = 1u;
	poolInfo.poolSizeCount = static_cast<std::uint32_t>(descriptorTypeCounts.size());
	poolInfo.pPoolSizes = descriptorTypeCounts.data();

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool)
	);
}

void DescriptorPool::AllocateDescriptors(
	VkDevice device, VkDescriptorSetLayout setLayout,
	VkDescriptorSet* descriptorSets
) const {
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1u;

	VkDescriptorSetLayout layouts[] = { setLayout };
	allocInfo.pSetLayouts = layouts;

	VkResult result;
	VK_THROW_FAILED(result,
		vkAllocateDescriptorSets(device, &allocInfo, descriptorSets)
	);
}
