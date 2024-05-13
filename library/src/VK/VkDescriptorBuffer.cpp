#include <VkDescriptorBuffer.hpp>

// Descriptor Set Layout
DescriptorSetLayout::~DescriptorSetLayout() noexcept
{
	SelfDestruct();
}

void DescriptorSetLayout::SelfDestruct() noexcept
{
	vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
}

void DescriptorSetLayout::Create()
{
	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{
		.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount  = static_cast<std::uint32_t>(std::size(m_layoutBindingFlags)),
		.pBindingFlags = std::data(m_layoutBindingFlags)
	};

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext        = &bindingFlagsCreateInfo,
		.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
		.bindingCount = static_cast<std::uint32_t>(std::size(m_layoutBindings)),
		.pBindings    = std::data(m_layoutBindings)
	};

	vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_layout);
}

void DescriptorSetLayout::AddBinding(
	std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags
) noexcept {
	m_layoutBindings.emplace_back(VkDescriptorSetLayoutBinding{
		.binding         = bindingIndex,
		.descriptorType  = type,
		.descriptorCount = descriptorCount,
		.stageFlags      = shaderFlags
	});

	m_layoutBindingFlags.emplace_back(bindingFlags);
}

void DescriptorSetLayout::CleanUpTempData()
{
	m_layoutBindings     = std::vector<VkDescriptorSetLayoutBinding>{};
	m_layoutBindingFlags = std::vector<VkDescriptorBindingFlags>{};
}

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

void VkDescriptorBuffer::CreateBuffer(const std::vector<std::uint32_t>& queueFamilyIndices/* = {} */)
{
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	m_setLayout.Create();
	m_setLayout.CleanUpTempData();

	VkDeviceSize layoutSizeInBytes = 0u;

	DescBuffer::vkGetDescriptorSetLayoutSizeEXT(m_device, m_setLayout.Get(), &layoutSizeInBytes);

	m_bufferFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

	m_descriptorBuffer.Create(layoutSizeInBytes, m_bufferFlags, queueFamilyIndices);
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
