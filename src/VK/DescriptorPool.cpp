#include <DescriptorPool.hpp>
#include <VKThrowMacros.hpp>
#include <vector>

DescriptorPool::DescriptorPool(VkDevice device) noexcept
	: m_deviceRef{ device }, m_descriptorPool{ VK_NULL_HANDLE } {}

DescriptorPool::~DescriptorPool() noexcept {
	vkDestroyDescriptorPool(m_deviceRef, m_descriptorPool, nullptr);
}

void DescriptorPool::AddDescriptorTypeLimit(
	VkDescriptorType type, std::uint32_t descriptorCount
) noexcept {
	m_typeMap[type] += descriptorCount;
}

void DescriptorPool::CreatePool(VkDevice device, std::uint32_t maxSets) {
	std::vector<VkDescriptorPoolSize> descriptorTypeCounts;
	for (auto& [type, descCount] : m_typeMap)
		descriptorTypeCounts.emplace_back(type, descCount);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	poolInfo.maxSets = maxSets;
	poolInfo.poolSizeCount = static_cast<std::uint32_t>(std::size(descriptorTypeCounts));
	poolInfo.pPoolSizes = std::data(descriptorTypeCounts);

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool)
	);
}

void DescriptorPool::AllocateDescriptors(
	VkDevice device, VkDescriptorSetLayout setLayout,
	std::uint32_t descriptorSetCount, VkDescriptorSet* descriptorSets
) const {
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = descriptorSetCount;

	VkDescriptorSetLayout layouts[] = { setLayout };
	allocInfo.pSetLayouts = layouts;

	VkResult result;
	VK_THROW_FAILED(result,
		vkAllocateDescriptorSets(device, &allocInfo, descriptorSets)
	);
}
