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

void DescriptorSetLayout::CreateLayout(
	std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags
)
{
	VkDescriptorSetLayoutBinding layoutBinding{
		.binding         = bindingIndex,
		.descriptorType  = type,
		.descriptorCount = descriptorCount,
		.stageFlags      = shaderFlags
	};

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
		.bindingCount = 1u,
		.pBindings    = &layoutBinding
	};

	vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_layout);
}

// Vk Descriptor Buffer
VkPhysicalDeviceDescriptorBufferPropertiesEXT VkDescriptorBuffer::s_descriptorInfo{
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
};

VkDescriptorBuffer& VkDescriptorBuffer::AddLayout(
	std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags
)
{
	DescriptorSetLayout setLayout{ m_device };
	setLayout.CreateLayout(bindingIndex, type, descriptorCount, shaderFlags);

	m_setLayouts.emplace_back(std::move(setLayout));

	return *this;
}

void VkDescriptorBuffer::CreateBuffer(const std::vector<std::uint32_t>& queueFamilyIndices/* = {} */)
{
	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	VkDeviceSize totalLayoutSizeInBytes = 0u;

	for (const auto& layout : m_setLayouts)
	{
		VkDeviceSize layoutSizeInBytes = 0u;

		DescBuffer::vkGetDescriptorSetLayoutSizeEXT(m_device, layout.Get(), &layoutSizeInBytes);

		m_layoutOffsets.emplace_back(totalLayoutSizeInBytes);
		totalLayoutSizeInBytes += layoutSizeInBytes;
	}

	m_bufferFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

	m_descriptorBuffer.Create(totalLayoutSizeInBytes, m_bufferFlags, queueFamilyIndices);
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

std::vector<VkDescriptorSetLayout> VkDescriptorBuffer::GetVkLayouts() const noexcept
{
	std::vector<VkDescriptorSetLayout> vkSetLayouts{ std::size(m_setLayouts) };

	for (size_t index = 0u; index < std::size(m_setLayouts); ++index)
		vkSetLayouts.at(index) = m_setLayouts.at(index).Get();

	return vkSetLayouts;
}
