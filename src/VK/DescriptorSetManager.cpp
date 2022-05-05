#include <DescriptorSetManager.hpp>
#include <VKThrowMacros.hpp>

DescriptorSetManager::DescriptorSetManager(VkDevice device)
	: m_deviceRef(device), m_descriptorSet(VK_NULL_HANDLE) {
	m_descriptorPool = std::make_unique<DescriptorPool>(device);
}

DescriptorSetManager::~DescriptorSetManager() noexcept {
	for (const auto layout : m_descriptorSetLayouts)
		vkDestroyDescriptorSetLayout(m_deviceRef, layout, nullptr);
	m_descriptorPool.reset();
}

DescriptorSetManager::VulkanDescriptorSetLayouts DescriptorSetManager::GetDescriptorSetLayouts(
) const noexcept {
	return m_descriptorSetLayouts;
}

VkDescriptorSet DescriptorSetManager::GetDescriptorSet() const noexcept {
	return m_descriptorSet;
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

		for (const BufferInfo& bufferInfo : m_bufferInfos)
			BindBuffer(device, bufferInfo);

		for (const ImageInfo& imageInfo : m_imageInfos)
			BindImageView(device, imageInfo);

		m_bufferInfos = std::vector<BufferInfo>();
		m_imageInfos = std::vector<ImageInfo>();
	}
}

void DescriptorSetManager::BindBuffer(
	VkDevice device, const BufferInfo& bufferInfo
) const noexcept {
	VkWriteDescriptorSet setWrite = {};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = bufferInfo.descriptorInfo.bindingSlot;
	setWrite.descriptorCount = bufferInfo.descriptorInfo.descriptorCount;
	setWrite.dstSet = m_descriptorSet;
	setWrite.descriptorType = bufferInfo.descriptorInfo.type;
	setWrite.dstArrayElement = 0u;
	setWrite.pBufferInfo = bufferInfo.buffers.data();

	vkUpdateDescriptorSets(device, 1u, &setWrite, 0u, nullptr);
}

void DescriptorSetManager::BindImageView(
	VkDevice device, const ImageInfo& imageInfo
) const noexcept {
	VkWriteDescriptorSet setWrite = {};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = imageInfo.descriptorInfo.bindingSlot;
	setWrite.descriptorCount = imageInfo.descriptorInfo.descriptorCount;
	setWrite.dstSet = m_descriptorSet;
	setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setWrite.dstArrayElement = 0u;
	setWrite.pImageInfo = imageInfo.images.data();

	vkUpdateDescriptorSets(device, 1u, &setWrite, 0u, nullptr);
}

void DescriptorSetManager::AddSetLayoutImage(
	VkDevice device, const DescriptorInfo& descInfo,
	VkShaderStageFlags shaderFlag
) {
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = descInfo.bindingSlot;
	layoutBinding.descriptorCount = descInfo.descriptorCount;
	layoutBinding.descriptorType = descInfo.type;
	layoutBinding.stageFlags = shaderFlag;

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 1u;
	createInfo.pBindings = &layoutBinding;
	createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	VkDescriptorBindingFlags bindlessFlag =
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo = {};
	flagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	flagsCreateInfo.bindingCount = 1u;
	flagsCreateInfo.pBindingFlags = &bindlessFlag;

	createInfo.pNext = &flagsCreateInfo;

	VkDescriptorSetLayout layout;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout)
	);

	m_descriptorSetLayouts.emplace_back(layout);

	m_descriptorPool->AddDescriptorTypeLimit(
		descInfo.type, descInfo.descriptorCount
	);
}

void DescriptorSetManager::AddSetLayoutBuffer(
	VkDevice device, const DescriptorInfo& descInfo,
	VkShaderStageFlags shaderFlag
) {
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = descInfo.bindingSlot;
	layoutBinding.descriptorCount = descInfo.descriptorCount;
	layoutBinding.descriptorType = descInfo.type;
	layoutBinding.stageFlags = shaderFlag;

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 1u;
	createInfo.pBindings = &layoutBinding;
	createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	VkDescriptorSetLayout layout;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout)
	);

	m_descriptorSetLayouts.emplace_back(layout);

	m_descriptorPool->AddDescriptorTypeLimit(
		descInfo.type, descInfo.descriptorCount
	);
}

void DescriptorSetManager::AddSetLayoutAndQueueForBinding(
	VkDevice device, DescriptorInfo descInfo,
	VkShaderStageFlags shaderFlag,
	std::vector<VkDescriptorBufferInfo>&& bufferInfo
) {
	AddSetLayoutBuffer(device, descInfo, shaderFlag);

	m_bufferInfos.emplace_back(std::move(descInfo), std::move(bufferInfo));
}

void DescriptorSetManager::AddSetLayoutAndQueueForBinding(
	VkDevice device, DescriptorInfo descInfo,
	VkShaderStageFlags shaderFlag,
	std::vector<VkDescriptorImageInfo>&& imageInfo
) {
	AddSetLayoutImage(device, descInfo, shaderFlag);

	m_imageInfos.emplace_back(std::move(descInfo), std::move(imageInfo));
}
