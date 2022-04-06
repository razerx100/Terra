#include <DescriptorPool.hpp>
#include <VKThrowMacros.hpp>

DescriptorPool::DescriptorPool(VkDevice device) noexcept
	: m_deviceRef(device), m_descriptorPool(VK_NULL_HANDLE) {
	m_typeMap = {
		-1, -1, -1
	};
}

DescriptorPool::~DescriptorPool() noexcept {
	vkDestroyDescriptorPool(m_deviceRef, m_descriptorPool, nullptr);
}

void DescriptorPool::AddDescriptorTypeLimit(
	DescriptorType type, std::uint32_t descriptorCount
) noexcept {
	size_t typeIndex = static_cast<size_t>(type);
	if (m_typeMap[typeIndex] == -1) {
		m_typeMap[typeIndex] = static_cast<std::int32_t>(m_descriptorTypeCounts.size());
		m_descriptorTypeCounts.emplace_back(static_cast<VkDescriptorType>(typeIndex + 6u), 1u);
	}
	else
		m_descriptorTypeCounts[m_typeMap[typeIndex]].descriptorCount += descriptorCount;
}

void DescriptorPool::CreatePool(VkDevice device, std::uint32_t setLayoutCount) {
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = 0u;
	poolInfo.maxSets = setLayoutCount;
	poolInfo.poolSizeCount = static_cast<std::uint32_t>(m_descriptorTypeCounts.size());
	poolInfo.pPoolSizes = m_descriptorTypeCounts.data();

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool)
	);
}

void DescriptorPool::AllocateDescriptors(
	VkDevice device, const VkDescriptorSetLayout* setLayouts,
	VkDescriptorSet* descriptorSets, std::uint32_t setCount
) const {
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = setCount;
	allocInfo.pSetLayouts = setLayouts;

	VkResult result;
	VK_THROW_FAILED(result,
		vkAllocateDescriptorSets(device, &allocInfo, descriptorSets)
	);
}
