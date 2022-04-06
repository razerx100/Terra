#include <DescriptorSetManager.hpp>
#include <VKThrowMacros.hpp>

DescriptorSetManager::DescriptorSetManager(VkDevice device) : m_deviceRef(device) {
	m_descriptorPool = std::make_unique<DescriptorPool>(device);
}

DescriptorSetManager::~DescriptorSetManager() noexcept {
	for (auto& layout : m_descriptorSetLayouts)
		vkDestroyDescriptorSetLayout(m_deviceRef, layout, nullptr);
	m_descriptorPool.reset();
}

VulkanDescriptorSetLayouts DescriptorSetManager::GetDescriptorSetLayouts() const noexcept {
	return m_descriptorSetLayouts;
}

VkDescriptorSet DescriptorSetManager::GetDescriptorSet() const noexcept {
	return m_descriptorSet;
}

void DescriptorSetManager::AddSetLayout(
	VkDevice device,
	VkDescriptorType type, VkShaderStageFlags shaderBits,
	std::uint32_t bindingSlot, std::uint32_t descriptorCount
) {
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = bindingSlot;
	layoutBinding.descriptorCount = descriptorCount;
	layoutBinding.descriptorType = type;
	layoutBinding.stageFlags = shaderBits;

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 1u;
	createInfo.pBindings = &layoutBinding;
	createInfo.flags = 0u;

	VkDescriptorSetLayout layout;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout)
	);

	m_descriptorSetLayouts.emplace_back(layout);

	DescriptorType localType;

	if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		localType = DescriptorType::UniformBufferDynamic;
	else if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		localType = DescriptorType::StorageBuffer;
	else
		localType = DescriptorType::UniformBuffer;

	m_descriptorPool->AddDescriptorTypeLimit(localType, descriptorCount);
}

void DescriptorSetManager::CreateDescriptorSets(VkDevice device) {
	if (!m_descriptorSetLayouts.empty()) {
		m_descriptorPool->CreatePool(
			device,
			static_cast<std::uint32_t>(m_descriptorSetLayouts.size())
		);
		m_descriptorPool->AllocateDescriptors(
			device, m_descriptorSetLayouts.data(),
			&m_descriptorSet
		);
	}
}
