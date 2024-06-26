#include <VkDescriptorBuffer.hpp>

// Vk Descriptor Buffer
VkPhysicalDeviceDescriptorBufferPropertiesEXT VkDescriptorBuffer::s_descriptorInfo{
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
};

VkDescriptorBuffer& VkDescriptorBuffer::AddBinding(
	std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags /* = 0u */
) noexcept {
	m_setLayout.AddBinding(bindingIndex, type, descriptorCount, shaderFlags, bindingFlags);

	return *this;
}

VkDescriptorBuffer& VkDescriptorBuffer::UpdateBinding(
	std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags /* = 0u */
) noexcept {
	m_setLayout.UpdateBinding(bindingIndex, type, descriptorCount, shaderFlags, bindingFlags);

	return *this;
}

void VkDescriptorBuffer::CreateBuffer(const std::vector<std::uint32_t>& queueFamilyIndices/* = {} */)
{
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	m_setLayout.Create();

	VkDeviceSize layoutSizeInBytes = 0u;

	DescBuffer::vkGetDescriptorSetLayoutSizeEXT(m_device, m_setLayout.Get(), &layoutSizeInBytes);

	m_descriptorBuffer.Create(layoutSizeInBytes, GetFlags(), queueFamilyIndices);
}

void VkDescriptorBuffer::RecreateBuffer(const std::vector<std::uint32_t>& queueFamilyIndices/* = {} */)
{
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	m_setLayout.Create();

	VkDeviceSize layoutSizeInBytes = 0u;

	DescBuffer::vkGetDescriptorSetLayoutSizeEXT(m_device, m_setLayout.Get(), &layoutSizeInBytes);

	Buffer newBuffer{ m_device, m_memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
	newBuffer.Create(layoutSizeInBytes, GetFlags(), queueFamilyIndices);

	// All of the old descriptors will be only copied if the new buffer is larger.
	const VkDeviceSize oldBufferSize = m_descriptorBuffer.Size();
	if (oldBufferSize && newBuffer.Size() > oldBufferSize)
	{
		memcpy(newBuffer.CPUHandle(), m_descriptorBuffer.CPUHandle(), m_descriptorBuffer.Size());
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

VkDeviceSize VkDescriptorBuffer::GetBindingOffset(std::uint32_t binding) const noexcept
{
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	VkDeviceSize offset = 0u;

	DescBuffer::vkGetDescriptorSetLayoutBindingOffsetEXT(
		m_device, m_setLayout.Get(), binding, &offset
	);

	return offset;
}

void* VkDescriptorBuffer::GetDescriptorAddress(
	std::uint32_t bindingIndex, size_t descriptorSize, std::uint32_t descriptorIndex
) const noexcept {
	return m_descriptorBuffer.CPUHandle() /* + layoutOffset */ + GetBindingOffset(bindingIndex)
		+ (descriptorIndex * descriptorSize);
}

void VkDescriptorBuffer::BindDescriptorBuffer(
	const VkDescriptorBuffer& descriptorBuffer, const VKCommandBuffer& cmdBuffer,
	const PipelineLayout& pipelineLayout
) noexcept {
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	VkCommandBuffer commandBuffer = cmdBuffer.Get();

	VkDescriptorBufferBindingInfoEXT bindingInfo{
		.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
		.address = descriptorBuffer.GpuPhysicalAddress(),
		.usage   = VkDescriptorBuffer::GetFlags()
	};

	DescBuffer::vkCmdBindDescriptorBuffersEXT(commandBuffer, 1u, &bindingInfo);

	VkPipelineLayout layout = pipelineLayout.Get();

	static constexpr std::uint32_t bindingInfoIndices[] = { 0u };
	static constexpr VkDeviceSize  bufferOffsets[]      = { 0u };

	DescBuffer::vkCmdSetDescriptorBufferOffsetsEXT(
		commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
		0u, 1u, bindingInfoIndices, bufferOffsets
	);
}
