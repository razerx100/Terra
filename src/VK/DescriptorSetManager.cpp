#include <DescriptorSetManager.hpp>
#include <VKThrowMacros.hpp>

#include <Terra.hpp>

// DescriptorSet Manager
DescriptorSetManager::DescriptorSetManager(VkDevice device, size_t bufferCount)
	: m_deviceRef{ device }, m_descriptorSets{ bufferCount, VK_NULL_HANDLE },
	m_descriptorSetLayouts{ bufferCount, VK_NULL_HANDLE }, m_descriptorPool{ device } {}

DescriptorSetManager::~DescriptorSetManager() noexcept {
	for(auto descriptorSetLayout : m_descriptorSetLayouts)
	vkDestroyDescriptorSetLayout(m_deviceRef, descriptorSetLayout, nullptr);
}

VkDescriptorSetLayout const* DescriptorSetManager::GetDescriptorSetLayouts() const noexcept {
	return std::data(m_descriptorSetLayouts);
}

VkDescriptorSet DescriptorSetManager::GetDescriptorSet(size_t index) const noexcept {
	return m_descriptorSets[index];
}

void DescriptorSetManager::CreateDescriptorSets(VkDevice device) {
	const std::uint32_t descriptorSetCount =
		static_cast<std::uint32_t>(std::size(m_descriptorSets));

	m_descriptorPool.CreatePool(device, descriptorSetCount);

	CreateSetLayouts(device);

	m_descriptorPool.AllocateDescriptors(
		device, std::data(m_descriptorSetLayouts), descriptorSetCount,
		std::data(m_descriptorSets)
	);

	for (const DescBufferInfo& bufferInfo : m_bufferInfos)
		for (size_t index = 0u; index < std::size(m_descriptorSets); ++index)
			BindBuffer(
				device, m_descriptorSets[index], bufferInfo.descriptorInfo,
				bufferInfo.buffers[index]
			);

	for (const DescImageInfo& imageInfo : m_imageInfos)
		for (auto descriptorSet : m_descriptorSets)
			BindImageView(device, descriptorSet, imageInfo);

	m_bufferInfos = std::vector<DescBufferInfo>();
	m_imageInfos = std::vector<DescImageInfo>();
}

void DescriptorSetManager::_addSetLayout(
	const DescriptorInfo& descInfo, VkShaderStageFlagBits shaderFlag
) {
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = descInfo.bindingSlot;
	layoutBinding.descriptorCount = descInfo.descriptorCount;
	layoutBinding.descriptorType = descInfo.type;
	layoutBinding.stageFlags = shaderFlag;

	m_layoutBindings.emplace_back(std::move(layoutBinding));

	const static std::uint32_t descriptorMultiplier =
		static_cast<std::uint32_t>(std::size(m_descriptorSets));

	m_descriptorPool.AddDescriptorTypeLimit(
		descInfo.type, descInfo.descriptorCount * descriptorMultiplier
	);

	VkDescriptorBindingFlags bindFlags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

	m_bindingFlags.emplace_back(bindFlags);
}

void DescriptorSetManager::AddSetLayout(
	const DescriptorInfo& descInfo, VkShaderStageFlagBits shaderFlag,
	std::vector<VkDescriptorBufferInfo>&& bufferInfo
) noexcept {
	_addSetLayout(descInfo, shaderFlag);

	m_bufferInfos.emplace_back(descInfo, std::move(bufferInfo));
}

void DescriptorSetManager::AddSetLayout(
	const DescriptorInfo& descInfo, VkShaderStageFlagBits shaderFlag,
	std::vector<VkDescriptorImageInfo>&& imageInfo
) noexcept {
	_addSetLayout(descInfo, shaderFlag);

	m_imageInfos.emplace_back(descInfo, std::move(imageInfo));
}

void DescriptorSetManager::CreateSetLayouts(VkDevice device) {
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<std::uint32_t>(std::size(m_layoutBindings));
	createInfo.pBindings = std::data(m_layoutBindings);
	createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo{};
	flagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	flagsCreateInfo.bindingCount = static_cast<std::uint32_t>(std::size(m_bindingFlags));
	flagsCreateInfo.pBindingFlags = std::data(m_bindingFlags);

	createInfo.pNext = &flagsCreateInfo;

	VkResult result{};
	for (auto& descriptorSetLayout : m_descriptorSetLayouts)
		VK_THROW_FAILED(result,
			vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout)
		);
}

void DescriptorSetManager::BindBuffer(
	VkDevice device, VkDescriptorSet descSet,
	const DescriptorInfo& descriptorInfo, const VkDescriptorBufferInfo& bufferInfo
) noexcept {
	VkWriteDescriptorSet setWrite{};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = descriptorInfo.bindingSlot;
	setWrite.descriptorCount = descriptorInfo.descriptorCount;
	setWrite.dstSet = descSet;
	setWrite.descriptorType = descriptorInfo.type;
	setWrite.dstArrayElement = 0u;
	setWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(device, 1u, &setWrite, 0u, nullptr);
}

void DescriptorSetManager::BindImageView(
	VkDevice device, VkDescriptorSet descSet, const DescImageInfo& imageInfo
) noexcept {
	VkWriteDescriptorSet setWrite{};
	setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	setWrite.dstBinding = imageInfo.descriptorInfo.bindingSlot;
	setWrite.descriptorCount = imageInfo.descriptorInfo.descriptorCount;
	setWrite.dstSet = descSet;
	setWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	setWrite.dstArrayElement = 0u;
	setWrite.pImageInfo = std::data(imageInfo.images);

	vkUpdateDescriptorSets(device, 1u, &setWrite, 0u, nullptr);
}

void DescriptorSetManager::AddDescriptorForBuffer(
	const VkResourceView& buffer, std::uint32_t bufferCount, std::uint32_t bindingSlot,
	VkDescriptorType descriptorType, VkShaderStageFlagBits shaderStage
) noexcept {
	DescriptorInfo descInfo{};
	descInfo.bindingSlot = bindingSlot;
	descInfo.descriptorCount = 1u;
	descInfo.type = descriptorType;

	std::vector<VkDescriptorBufferInfo> bufferInfos;

	for (VkDeviceSize index = 0u; index < bufferCount; ++index) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer.GetResource();
		bufferInfo.offset = buffer.GetMemoryOffset(index);
		bufferInfo.range = buffer.GetSubAllocationSize();

		bufferInfos.emplace_back(std::move(bufferInfo));
	}

	Terra::descriptorSet->AddSetLayout(descInfo, shaderStage, std::move(bufferInfos));
}
