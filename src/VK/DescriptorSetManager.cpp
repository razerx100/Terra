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

VulkanDescriptorSetLayouts DescriptorSetManager::GetDescriptorSetLayouts() const noexcept {
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
	VkDescriptorBufferInfo descBufferInfo = {};
	descBufferInfo.buffer = bufferInfo.buffer;
	descBufferInfo.offset = 0u;
	descBufferInfo.range = bufferInfo.size;

	VkWriteDescriptorSet setWrite = {};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = bufferInfo.descriptorInfo.bindingSlot;
	setWrite.descriptorCount = bufferInfo.descriptorInfo.descriptorCount;
	setWrite.dstSet = m_descriptorSet;
	setWrite.descriptorType = bufferInfo.descriptorInfo.type;
	setWrite.dstArrayElement = 0u;
	setWrite.pBufferInfo = &descBufferInfo;

	vkUpdateDescriptorSets(device, 1u, &setWrite, 0u, nullptr);
}

void DescriptorSetManager::BindImageView(
	VkDevice device, const ImageInfo& imageInfo
) const noexcept {
	VkDescriptorImageInfo descImageInfo = {};
	descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	descImageInfo.imageView = imageInfo.imageView;
	descImageInfo.sampler = imageInfo.sampler;

	VkWriteDescriptorSet setWrite = {};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = imageInfo.descriptorInfo.bindingSlot;
	setWrite.descriptorCount = imageInfo.descriptorInfo.descriptorCount;
	setWrite.dstSet = m_descriptorSet;
	setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setWrite.dstArrayElement = 0u;
	setWrite.pImageInfo = &descImageInfo;

	vkUpdateDescriptorSets(device, 1u, &setWrite, 0u, nullptr);
}

void DescriptorSetManager::AddSetLayout(
	const BindBufferInputInfo& inputInfo
) {
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = inputInfo.descriptorInfo.bindingSlot;
	layoutBinding.descriptorCount = inputInfo.descriptorInfo.descriptorCount;
	layoutBinding.descriptorType = inputInfo.descriptorInfo.type;
	layoutBinding.stageFlags = inputInfo.shaderBits;

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 1u;
	createInfo.pBindings = &layoutBinding;
	createInfo.flags = 0u;

	VkDescriptorSetLayout layout;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorSetLayout(inputInfo.device, &createInfo, nullptr, &layout)
	);

	m_descriptorSetLayouts.emplace_back(layout);

	m_descriptorPool->AddDescriptorTypeLimit(
		inputInfo.descriptorInfo.type, inputInfo.descriptorInfo.descriptorCount
	);

	BufferInfo bufferInfo = {};
	bufferInfo.buffer = inputInfo.buffer;
	bufferInfo.size = inputInfo.bufferSize;
	bufferInfo.descriptorInfo = inputInfo.descriptorInfo;

	m_bufferInfos.emplace_back(std::move(bufferInfo));
}

void DescriptorSetManager::AddSetLayout(
	const BindImageViewInputInfo& inputInfo
) {
	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.binding = inputInfo.descriptorInfo.bindingSlot;
	layoutBinding.descriptorCount = inputInfo.descriptorInfo.descriptorCount;
	layoutBinding.descriptorType = inputInfo.descriptorInfo.type;
	layoutBinding.stageFlags = inputInfo.shaderBits;

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 1u;
	createInfo.pBindings = &layoutBinding;
	createInfo.flags = 0u;

	VkDescriptorSetLayout layout;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDescriptorSetLayout(inputInfo.device, &createInfo, nullptr, &layout)
	);

	m_descriptorSetLayouts.emplace_back(layout);

	m_descriptorPool->AddDescriptorTypeLimit(
		inputInfo.descriptorInfo.type, inputInfo.descriptorInfo.descriptorCount
	);

	ImageInfo imageInfo = {};
	imageInfo.imageView = inputInfo.imageView;
	imageInfo.sampler = inputInfo.sampler;
	imageInfo.descriptorInfo = inputInfo.descriptorInfo;

	m_imageInfos.emplace_back(std::move(imageInfo));
}

