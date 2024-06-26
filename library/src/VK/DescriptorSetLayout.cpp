#include <DescriptorSetLayout.hpp>

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
	else
		AddBinding(bindingIndex, type, descriptorCount, shaderFlags, bindingFlags);
}
