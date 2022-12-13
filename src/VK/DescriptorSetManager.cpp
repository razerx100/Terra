#include <DescriptorSetManager.hpp>

#include <Terra.hpp>

// DescriptorSet Manager
DescriptorSetManager::DescriptorSetManager(VkDevice device, size_t bufferCount)
	: m_deviceRef{ device }, m_descriptorSets{ bufferCount, VK_NULL_HANDLE },
	m_descriptorSetLayouts{ bufferCount, VK_NULL_HANDLE }, m_descriptorPool{ device } {}

DescriptorSetManager::~DescriptorSetManager() noexcept {
	for (auto descriptorSetLayout : m_descriptorSetLayouts)
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

	assert(descriptorSetCount && "DescriptorSetCount is zero.");

	m_descriptorPool.CreatePool(device, descriptorSetCount);

	CreateSetLayouts(device);

	m_descriptorPool.AllocateDescriptors(
		device, std::data(m_descriptorSetLayouts), descriptorSetCount,
		std::data(m_descriptorSets)
	);

	for (auto& descInst : m_descriptorInstances)
		descInst->UpdateDescriptors(device, m_descriptorSets);

	m_descriptorInstances = std::vector<std::unique_ptr<DescriptorInstance>>();
}

void DescriptorSetManager::AddSetLayout(
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

void DescriptorSetManager::AddBuffersSplit(
	const DescriptorInfo& descInfo, std::vector<VkDescriptorBufferInfo> bufferInfos,
	VkShaderStageFlagBits shaderFlag
) noexcept {
	AddSetLayout(descInfo, shaderFlag);

	auto descInst = std::make_unique<DescriptorInstanceBuffer>();
	descInst->AddBuffersSplit(descInfo, std::move(bufferInfos));

	m_descriptorInstances.emplace_back(std::move(descInst));
}

void DescriptorSetManager::AddBuffersContiguous(
	const DescriptorInfo& descInfo, std::vector<VkDescriptorBufferInfo> bufferInfos,
	VkShaderStageFlagBits shaderFlag
) noexcept {
	AddSetLayout(descInfo, shaderFlag);

	auto descInst = std::make_unique<DescriptorInstanceBuffer>();
	descInst->AddBuffersContiguous(descInfo, std::move(bufferInfos));

	m_descriptorInstances.emplace_back(std::move(descInst));
}

void DescriptorSetManager::AddImagesSplit(
	const DescriptorInfo& descInfo, std::vector<VkDescriptorImageInfo> imageInfos,
	VkShaderStageFlagBits shaderFlag
) noexcept {
	AddSetLayout(descInfo, shaderFlag);

	auto descInst = std::make_unique<DescriptorInstanceImage>();
	descInst->AddImagesSplit(descInfo, std::move(imageInfos));

	m_descriptorInstances.emplace_back(std::move(descInst));
}

void DescriptorSetManager::AddImagesContiguous(
	const DescriptorInfo& descInfo, std::vector<VkDescriptorImageInfo> imageInfos,
	VkShaderStageFlagBits shaderFlag
) noexcept {
	AddSetLayout(descInfo, shaderFlag);

	auto descInst = std::make_unique<DescriptorInstanceImage>();
	descInst->AddImagesContiguous(descInfo, std::move(imageInfos));

	m_descriptorInstances.emplace_back(std::move(descInst));
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

	for (auto& descriptorSetLayout : m_descriptorSetLayouts)
		vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout);
}

// Descriptor Instance base
void DescriptorSetManager::DescriptorInstance::UpdateDescriptors(
	VkDevice device, const std::vector<VkDescriptorSet>& descSets
) const noexcept {
	auto setWrites = PopulateWriteDescSets(descSets);

	vkUpdateDescriptorSets(
		device, static_cast<std::uint32_t>(std::size(setWrites)), std::data(setWrites), 0u,
		nullptr
	);
}

// Descriptor Instance Buffer
void DescriptorSetManager::DescriptorInstanceBuffer::AddBuffersSplit(
	const DescriptorInfo& descInfo, std::vector<VkDescriptorBufferInfo> bufferInfos
) noexcept {
	m_descriptorInfo = descInfo;
	m_bufferInfos = std::move(bufferInfos);
	m_isSplit = true;
}

void DescriptorSetManager::DescriptorInstanceBuffer::AddBuffersContiguous(
	const DescriptorInfo& descInfo, std::vector<VkDescriptorBufferInfo> bufferInfos
) noexcept {
	m_descriptorInfo = descInfo;
	m_bufferInfos = std::move(bufferInfos);
	m_isSplit = false;
}

std::vector<VkWriteDescriptorSet> DescriptorSetManager::DescriptorInstanceBuffer::PopulateWriteDescSets(
	const std::vector<VkDescriptorSet>& descSets
) const noexcept {
	std::vector<VkWriteDescriptorSet> setWrites;

	if (m_isSplit) {
		assert(
			std::size(m_bufferInfos) == std::size(descSets)
			&& "More buffers than descriptor Sets."
		);

		for (size_t index = 0u; index < std::size(m_bufferInfos); ++index) {
			VkWriteDescriptorSet setWrite{};
			setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWrite.dstBinding = m_descriptorInfo.bindingSlot;
			setWrite.descriptorCount = m_descriptorInfo.descriptorCount;
			setWrite.dstSet = descSets[index];
			setWrite.descriptorType = m_descriptorInfo.type;
			setWrite.dstArrayElement = 0u;
			setWrite.pBufferInfo = &m_bufferInfos[index];

			setWrites.emplace_back(setWrite);
		}
	}
	else
		for (auto descSet : descSets) {
			VkWriteDescriptorSet setWrite{};
			setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWrite.dstBinding = m_descriptorInfo.bindingSlot;
			setWrite.descriptorCount = m_descriptorInfo.descriptorCount;
			setWrite.dstSet = descSet;
			setWrite.descriptorType = m_descriptorInfo.type;
			setWrite.dstArrayElement = 0u;
			setWrite.pBufferInfo = std::data(m_bufferInfos);

			setWrites.emplace_back(setWrite);
		}

	return setWrites;
}

// Descriptor Instance Image
void DescriptorSetManager::DescriptorInstanceImage::AddImagesSplit(
	const DescriptorInfo& descInfo, std::vector<VkDescriptorImageInfo> imageInfos
) noexcept {
	m_descriptorInfo = descInfo;
	m_imageInfos = std::move(imageInfos);
	m_isSplit = true;
}

void DescriptorSetManager::DescriptorInstanceImage::AddImagesContiguous(
	const DescriptorInfo& descInfo, std::vector<VkDescriptorImageInfo> imageInfos
) noexcept {
	m_descriptorInfo = descInfo;
	m_imageInfos = std::move(imageInfos);
	m_isSplit = false;
}

std::vector<VkWriteDescriptorSet> DescriptorSetManager::DescriptorInstanceImage::PopulateWriteDescSets(
	const std::vector<VkDescriptorSet>& descSets
) const noexcept {
	std::vector<VkWriteDescriptorSet> setWrites;

	if (m_isSplit) {
		assert(
			std::size(m_imageInfos) == std::size(descSets)
			&& "More images than descriptor Sets."
		);

		for (size_t index = 0u; index < std::size(m_imageInfos); ++index) {
			VkWriteDescriptorSet setWrite{};
			setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWrite.dstBinding = m_descriptorInfo.bindingSlot;
			setWrite.descriptorCount = m_descriptorInfo.descriptorCount;
			setWrite.dstSet = descSets[index];
			setWrite.descriptorType = m_descriptorInfo.type;
			setWrite.dstArrayElement = 0u;
			setWrite.pImageInfo = &m_imageInfos[index];

			setWrites.emplace_back(setWrite);
		}
	}
	else
		for (auto descSet : descSets) {
			VkWriteDescriptorSet setWrite{};
			setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			setWrite.dstBinding = m_descriptorInfo.bindingSlot;
			setWrite.descriptorCount = m_descriptorInfo.descriptorCount;
			setWrite.dstSet = descSet;
			setWrite.descriptorType = m_descriptorInfo.type;
			setWrite.dstArrayElement = 0u;
			setWrite.pImageInfo = std::data(m_imageInfos);

			setWrites.emplace_back(setWrite);
		}

	return setWrites;
}
