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

	if (m_layout != VK_NULL_HANDLE)
		SelfDestruct();

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

void DescriptorSetLayout::UpdateBinding(
	std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
	VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags
) noexcept {
	auto result = std::ranges::find(
		m_layoutBindings, bindingIndex,
		[](const VkDescriptorSetLayoutBinding& binding)
		{
			return binding.binding;
		}
	);

	if (result != std::end(m_layoutBindings))
	{
		const auto indexInContainers = static_cast<size_t>(
			std::distance(std::begin(m_layoutBindings), result)
		);

		m_layoutBindings.at(indexInContainers) = VkDescriptorSetLayoutBinding{
			.binding         = bindingIndex,
			.descriptorType  = type,
			.descriptorCount = descriptorCount,
			.stageFlags      = shaderFlags
		};

		m_layoutBindingFlags.at(indexInContainers) = bindingFlags;
	}
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
