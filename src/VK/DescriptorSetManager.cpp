#include <DescriptorSetManager.hpp>
#include <VKThrowMacros.hpp>

void BindBuffer(
	VkDevice device, VkDescriptorSet descSet,
	const DescBufferInfo& bufferInfo
) noexcept {
	VkWriteDescriptorSet setWrite = {};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = bufferInfo.descriptorInfo.bindingSlot;
	setWrite.descriptorCount = bufferInfo.descriptorInfo.descriptorCount;
	setWrite.dstSet = descSet;
	setWrite.descriptorType = bufferInfo.descriptorInfo.type;
	setWrite.dstArrayElement = 0u;
	setWrite.pBufferInfo = bufferInfo.buffers.data();

	vkUpdateDescriptorSets(device, 1u, &setWrite, 0u, nullptr);
}

void BindImageView(
	VkDevice device, VkDescriptorSet descSet,
	const DescImageInfo& imageInfo
) noexcept {
	VkWriteDescriptorSet setWrite = {};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = imageInfo.descriptorInfo.bindingSlot;
	setWrite.descriptorCount = imageInfo.descriptorInfo.descriptorCount;
	setWrite.dstSet = descSet;
	setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setWrite.dstArrayElement = 0u;
	setWrite.pImageInfo = imageInfo.images.data();

	vkUpdateDescriptorSets(device, 1u, &setWrite, 0u, nullptr);
}

// DescriptorSet Manager

DescriptorSetManager::DescriptorSetManager(VkDevice device)
	: m_deviceRef(device), m_descriptorSet(VK_NULL_HANDLE),
	m_descriptorSetLayout(VK_NULL_HANDLE) {
	m_descriptorPool = std::make_unique<DescriptorPool>(device);
}

DescriptorSetManager::~DescriptorSetManager() noexcept {
	vkDestroyDescriptorSetLayout(m_deviceRef, m_descriptorSetLayout, nullptr);
	m_descriptorPool.reset();
}

VkDescriptorSetLayout DescriptorSetManager::GetDescriptorSetLayout() const noexcept {
	return m_descriptorSetLayout;
}

VkDescriptorSet DescriptorSetManager::GetDescriptorSet() const noexcept {
	return m_descriptorSet;
}

void DescriptorSetManager::CreateDescriptorSets(VkDevice device) {
	m_descriptorPool->CreatePool(device);

	CreateSetLayout(device);

	m_descriptorPool->AllocateDescriptors(
		device, m_descriptorSetLayout,
		&m_descriptorSet
	);

	for (const DescBufferInfo& bufferInfo : m_bufferInfos)
		BindBuffer(device, m_descriptorSet, bufferInfo);

	for (const DescImageInfo& imageInfo : m_imageInfos)
		BindImageView(device, m_descriptorSet, imageInfo);

	m_bufferInfos = std::vector<DescBufferInfo>();
	m_imageInfos = std::vector<DescImageInfo>();
}

void DescriptorSetManager::AddSetLayout(
	const DescriptorInfo& descInfo, VkShaderStageFlags shaderFlag
) {
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = descInfo.bindingSlot;
	layoutBinding.descriptorCount = descInfo.descriptorCount;
	layoutBinding.descriptorType = descInfo.type;
	layoutBinding.stageFlags = shaderFlag;

	m_layoutBindings.emplace_back(std::move(layoutBinding));

	m_descriptorPool->AddDescriptorTypeLimit(
		descInfo.type, descInfo.descriptorCount
	);
}

void DescriptorSetManager::AddSetLayoutAndQueueForBinding(
	DescriptorInfo descInfo, VkShaderStageFlags shaderFlag,
	std::vector<VkDescriptorBufferInfo>&& bufferInfo
) {
	AddSetLayout(descInfo, shaderFlag);

	m_bufferInfos.emplace_back(std::move(descInfo), std::move(bufferInfo));
}

void DescriptorSetManager::AddSetLayoutAndQueueForBinding(
	DescriptorInfo descInfo, VkShaderStageFlags shaderFlag,
	std::vector<VkDescriptorImageInfo>&& imageInfo
) {
	AddSetLayout(descInfo, shaderFlag);

	m_imageInfos.emplace_back(std::move(descInfo), std::move(imageInfo));
}

void DescriptorSetManager::CreateSetLayout(VkDevice device) {
	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<std::uint32_t>(std::size(m_layoutBindings));
	createInfo.pBindings = std::data(m_layoutBindings);
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

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &m_descriptorSetLayout)
	);
}
