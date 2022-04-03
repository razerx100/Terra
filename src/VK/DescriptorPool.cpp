#include <DescriptorPool.hpp>
#include <VKThrowMacros.hpp>

DescriptorPool::DescriptorPool(VkDevice device) noexcept
	: m_deviceRef(device) {
	m_descriptorTypeCounts = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0u},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0u}
	};
}

DescriptorPool::~DescriptorPool() noexcept {
	vkDestroyDescriptorPool(m_deviceRef, m_descriptorPool, nullptr);
}

void DescriptorPool::AddDescriptorTypeLimit(
	DescriptorType type, std::uint32_t descriptorCount
) noexcept {
	m_descriptorTypeCounts[static_cast<size_t>(type)].descriptorCount += descriptorCount;
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
