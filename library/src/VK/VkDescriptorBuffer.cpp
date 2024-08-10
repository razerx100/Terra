#include <VkDescriptorBuffer.hpp>
#include <numeric>

// Vk Descriptor Buffer
VkPhysicalDeviceDescriptorBufferPropertiesEXT VkDescriptorBuffer::s_descriptorInfo{
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
};

VkDescriptorBuffer::VkDescriptorBuffer(
	VkDevice device, MemoryManager* memoryManager, std::uint32_t setLayoutCount
) : m_device{ device }, m_memoryManager{ memoryManager }, m_layoutOffsets(setLayoutCount, 0u),
	m_layoutIndices(setLayoutCount, 0u), m_setLayouts{},
	m_descriptorBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }
{
	std::iota(std::begin(m_layoutIndices), std::end(m_layoutIndices), 0u);

	for (size_t index = 0u; index < setLayoutCount; ++index)
		m_setLayouts.emplace_back(device);
}

VkDescriptorBuffer& VkDescriptorBuffer::AddBinding(
	std::uint32_t bindingIndex, size_t setLayoutIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags /* = 0u */
) noexcept {
	m_setLayouts.at(setLayoutIndex).AddBinding(
		bindingIndex, type, descriptorCount, shaderFlags, bindingFlags
	);

	return *this;
}

VkDescriptorBuffer& VkDescriptorBuffer::UpdateBinding(
	std::uint32_t bindingIndex, size_t setLayoutIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags /* = 0u */
) noexcept {
	m_setLayouts.at(setLayoutIndex).UpdateBinding(
		bindingIndex, type, descriptorCount, shaderFlags, bindingFlags
	);

	return *this;
}

void VkDescriptorBuffer::_createBuffer(
	Buffer& descriptorBuffer, const std::vector<std::uint32_t>& queueFamilyIndices
) {
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	VkDeviceSize layoutSizeInBytes = 0u;

	for (size_t index = 0u; index < std::size(m_setLayouts); ++index)
	{
		DescriptorSetLayout& setLayout = m_setLayouts[index];
		VkDeviceSize currentLayoutSize = 0u;

		setLayout.Create();

		DescBuffer::vkGetDescriptorSetLayoutSizeEXT(m_device, setLayout.Get(), &currentLayoutSize);

		m_layoutOffsets[index] = layoutSizeInBytes;
		layoutSizeInBytes     += currentLayoutSize;
	}

	descriptorBuffer.Create(layoutSizeInBytes, GetFlags(), queueFamilyIndices);
}

void VkDescriptorBuffer::CreateBuffer(const std::vector<std::uint32_t>& queueFamilyIndices/* = {} */)
{
	_createBuffer(m_descriptorBuffer, queueFamilyIndices);
}

void VkDescriptorBuffer::RecreateBuffer(const std::vector<std::uint32_t>& queueFamilyIndices/* = {} */)
{
	Buffer newBuffer{ m_device, m_memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	_createBuffer(newBuffer, queueFamilyIndices);

	const VkDeviceSize oldBufferSize = m_descriptorBuffer.BufferSize();

	// Usually we should only recreate when have added more layout bindings, which should cause the
	// size to increase.
	// It would be kinda useless to recreate when nothing was added or removed and the size is the
	// same.
	// But the copying can't be done if we change a layout to have less items, as the size will
	// decrease and so the copying won't be possible. In that case, we will have to recreate the
	// descriptors again.
	if (oldBufferSize && newBuffer.BufferSize() >= oldBufferSize)
		memcpy(newBuffer.CPUHandle(), m_descriptorBuffer.CPUHandle(), m_descriptorBuffer.BufferSize());

	m_descriptorBuffer = std::move(newBuffer);
}

void VkDescriptorBuffer::SetDescriptorBufferInfo(VkPhysicalDevice physicalDevice) noexcept
{
	VkPhysicalDeviceProperties2 physicalDeviceProp2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = &s_descriptorInfo
	};

	vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProp2);
}

VkDescriptorAddressInfoEXT VkDescriptorBuffer::GetBufferDescAddressInfo(
	VkDeviceAddress bufferStartingAddress, VkDeviceSize bufferSize, VkFormat texelBufferFormat
) noexcept {
	VkDescriptorAddressInfoEXT bufferDescAddressInfo{
		.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
		.address = bufferStartingAddress,
		.range   = bufferSize,
		.format  = texelBufferFormat
	};

	return bufferDescAddressInfo;
}

VkDeviceSize VkDescriptorBuffer::GetBindingOffset(
	std::uint32_t binding, size_t setLayoutIndex
) const noexcept {
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	VkDeviceSize offset = 0u;

	DescBuffer::vkGetDescriptorSetLayoutBindingOffsetEXT(
		m_device, m_setLayouts.at(setLayoutIndex).Get(), binding, &offset
	);

	return offset;
}

void* VkDescriptorBuffer::GetDescriptorAddress(
	std::uint32_t bindingIndex, size_t setLayoutIndex, size_t descriptorSize,
	std::uint32_t descriptorIndex
) const noexcept {
	return m_descriptorBuffer.CPUHandle() + m_layoutOffsets.at(setLayoutIndex) +
		+ GetBindingOffset(bindingIndex, setLayoutIndex) + (descriptorIndex * descriptorSize);
}

void VkDescriptorBuffer::BindDescriptorBuffer(
	const VkDescriptorBuffer& descriptorBuffer, const VKCommandBuffer& cmdBuffer,
	VkPipelineBindPoint bindPoint, const PipelineLayout& pipelineLayout
) {
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	VkCommandBuffer commandBuffer = cmdBuffer.Get();

	VkDescriptorBufferBindingInfoEXT bindingInfo{
		.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
		.address = descriptorBuffer.GpuPhysicalAddress(),
		.usage   = VkDescriptorBuffer::GetFlags()
	};

	DescBuffer::vkCmdBindDescriptorBuffersEXT(commandBuffer, 1u, &bindingInfo);

	const std::vector<VkDeviceSize>& setLayoutOffsets  = descriptorBuffer.GetLayoutOffsets();
	const std::vector<std::uint32_t>& setLayoutIndices = descriptorBuffer.GetLayoutIndices();

	VkPipelineLayout layout      = pipelineLayout.Get();
	const auto setCount          = static_cast<std::uint32_t>(std::size(setLayoutOffsets));
	const std::uint32_t firstSet = setLayoutIndices.front();

	DescBuffer::vkCmdSetDescriptorBufferOffsetsEXT(
		commandBuffer, bindPoint, layout, firstSet, setCount,
		std::data(setLayoutIndices), std::data(setLayoutOffsets)
	);
}
