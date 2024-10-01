#include <VkDescriptorBuffer.hpp>

// Vk Descriptor Buffer
VkPhysicalDeviceDescriptorBufferPropertiesEXT VkDescriptorBuffer::s_descriptorInfo{
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
};

VkDescriptorBuffer::VkDescriptorBuffer(
	VkDevice device, MemoryManager* memoryManager, std::uint32_t setLayoutCount
) : m_device{ device }, m_memoryManager{ memoryManager }, m_setLayouts{},
	// Since we have all of the SetLayouts in a single buffer, the indices are all the same, 0.
	m_bufferIndices(setLayoutCount, 0u),
	m_layoutOffsets(setLayoutCount, 0u),
	m_descriptorBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }
{
	for (size_t index = 0u; index < setLayoutCount; ++index)
		m_setLayouts.emplace_back(device);
}

VkDescriptorBuffer& VkDescriptorBuffer::AddBinding(
	std::uint32_t bindingIndex, size_t setLayoutIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags /* = 0u */
) noexcept {
	m_setLayouts[setLayoutIndex].UpdateBinding(
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

void VkDescriptorBuffer::IncreaseDescriptorCount(
	std::uint32_t bindingIndex, size_t setLayoutIndex, VkDescriptorType type,
	std::uint32_t oldDescriptorCount, std::uint32_t newDescriptorCount,
	const std::vector<std::uint32_t>& queueFamilyIndices /* = {} */
) {
	// Skip if this is called before the descriptorBuffer has been created.
	if (m_descriptorBuffer.Get() == VK_NULL_HANDLE)
		return;

	Buffer newBuffer{ m_device, m_memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	const VkDeviceSize oldDescriptorOffset = GetDescriptorOffset(
		bindingIndex, setLayoutIndex, GetDescriptorSize(type), oldDescriptorCount
	);

	const std::vector<VkDeviceSize> oldLayoutOffsets = m_layoutOffsets;

	_createBuffer(newBuffer, queueFamilyIndices);

	const VkDeviceSize newDescriptorOffset = GetDescriptorOffset(
		bindingIndex, setLayoutIndex, GetDescriptorSize(type), newDescriptorCount
	);

	// Usually we should only recreate when have added more layout bindings, which should cause the
	// size to increase.
	// It would be kinda useless to recreate when nothing was added or removed and the size is the
	// same.
	// But the copying can't be done if we change a layout to have less items, as the size will
	// decrease and so the copying won't be possible. In that case, we will have to recreate the
	// descriptors again.
	assert(
		newBuffer.BufferSize() >= m_descriptorBuffer.BufferSize()
		&& "The new descriptor buffer has a smaller size. So, copying the descriptors isn't possible."
	);

	{
		// Copy the other SetLayouts first, as they should be unchanged.
		const size_t layoutCount = std::size(oldLayoutOffsets);

		for (size_t index = 0u; index < layoutCount; ++index)
		{
			const size_t difference     = layoutCount - index;
			const size_t oldLayoutStart = static_cast<size_t>(oldLayoutOffsets[index]);
			const size_t newLayoutStart = static_cast<size_t>(m_layoutOffsets[index]);
			size_t layoutSize           = 0u;

			if (difference > 1u)
				layoutSize = static_cast<size_t>(oldLayoutOffsets[index + 1] - oldLayoutStart);
			else
				layoutSize = static_cast<size_t>(m_descriptorBuffer.BufferSize() - oldLayoutStart);

			// For the changed Layout.
			if (index == setLayoutIndex)
			{
				// The first unchanged part.
				const auto firstUnchangedPartSize
					= static_cast<size_t>(oldDescriptorOffset - oldLayoutStart);

				memcpy(
					newBuffer.CPUHandle() + newLayoutStart,
					m_descriptorBuffer.CPUHandle() + oldLayoutStart,
					firstUnchangedPartSize
				);

				size_t newLayoutSize = 0u;

				if (difference > 1u)
					newLayoutSize = static_cast<size_t>(m_layoutOffsets[index + 1] - newLayoutStart);
				else
					newLayoutSize = static_cast<size_t>(newBuffer.BufferSize() - newLayoutStart);

				//The second unchanged part.
				// The layout size might be bigger than the actual size held by the bindings.
				// So, can't just copy the oldLastPart.
				const auto oldLayoutLastPartSize = static_cast<size_t>(
					oldLayoutStart + layoutSize - oldDescriptorOffset
				);
				const auto newLayoutLastPartSize = static_cast<size_t>(
					newLayoutStart + newLayoutSize - newDescriptorOffset
				);

				const size_t secondUnchangedPartSize = std::min(
					oldLayoutLastPartSize, newLayoutLastPartSize
				);

				memcpy(
					newBuffer.CPUHandle() + newDescriptorOffset,
					m_descriptorBuffer.CPUHandle() + oldDescriptorOffset,
					secondUnchangedPartSize
				);
			}
			else
				memcpy(
					newBuffer.CPUHandle() + newLayoutStart,
					m_descriptorBuffer.CPUHandle() + oldLayoutStart,
					layoutSize
				);
		}
	}

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
		m_device, m_setLayouts[setLayoutIndex].Get(), binding, &offset
	);

	return offset;
}

VkDeviceSize VkDescriptorBuffer::GetDescriptorOffset(
	std::uint32_t bindingIndex, size_t setLayoutIndex, size_t descriptorSize,
	std::uint32_t descriptorIndex
) const noexcept {
	return m_layoutOffsets[setLayoutIndex] +
		+ GetBindingOffset(bindingIndex, setLayoutIndex) + (descriptorIndex * descriptorSize);
}

void* VkDescriptorBuffer::GetDescriptorAddress(
	std::uint32_t bindingIndex, size_t setLayoutIndex, size_t descriptorSize,
	std::uint32_t descriptorIndex
) const noexcept {
	return m_descriptorBuffer.CPUHandle() + GetDescriptorOffset(
		bindingIndex, setLayoutIndex, descriptorSize, descriptorIndex
	);
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

	VkPipelineLayout layout                           = pipelineLayout.Get();
	const std::vector<VkDeviceSize>& setLayoutOffsets = descriptorBuffer.GetLayoutOffsets();
	const std::vector<std::uint32_t>& bufferIndices   = descriptorBuffer.GetBufferIndices();

	// Since we have all of the SetLayouts in a single buffer, the indices should all be 0.
	// And also the firstSet should be 0 and should progressively increase, which would be
	// done by the API call below.
	constexpr std::uint32_t firstSet = 0u;
	const auto setCount              = static_cast<std::uint32_t>(std::size(setLayoutOffsets));

	DescBuffer::vkCmdSetDescriptorBufferOffsetsEXT(
		commandBuffer, bindPoint, layout, firstSet, setCount,
		std::data(bufferIndices), std::data(setLayoutOffsets)
	);
}
