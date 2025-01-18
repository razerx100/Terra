#include <PipelineLayout.hpp>

PipelineLayout::~PipelineLayout() noexcept
{
	SelfDestruct();
}

void PipelineLayout::SelfDestruct() noexcept
{
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
}

void PipelineLayout::AddPushConstantRange(
	VkShaderStageFlags shaderFlag, std::uint32_t rangeSize
) noexcept {
	VkPushConstantRange range{
		.stageFlags = shaderFlag,
		.offset     = m_pushConstantOffset,
		.size       = rangeSize
	};

	m_pushConstantOffset += rangeSize;

	m_pushRanges.emplace_back(range);
}

void PipelineLayout::Create(
	VkDescriptorSetLayout const* setLayouts, std::uint32_t layoutCount
) {
	VkPipelineLayoutCreateInfo createInfo{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount         = layoutCount,
		.pSetLayouts            = setLayouts,
		.pushConstantRangeCount = static_cast<std::uint32_t>(std::size(m_pushRanges)),
		.pPushConstantRanges    = std::data(m_pushRanges)
	};

	if (m_pipelineLayout != VK_NULL_HANDLE)
		SelfDestruct();

	vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_pipelineLayout);
}

void PipelineLayout::Create(const std::vector<VkDescriptorSetLayout>& setLayouts)
{
	Create(std::data(setLayouts), static_cast<std::uint32_t>(std::size(setLayouts)));
}

void PipelineLayout::Create(const std::vector<DescriptorSetLayout>& setLayouts)
{
	const size_t layoutCount = std::size(setLayouts);
	std::vector<VkDescriptorSetLayout> vkSetLayouts{ layoutCount, VK_NULL_HANDLE };

	for (size_t index = 0u; index < layoutCount; ++index)
		vkSetLayouts[index] = setLayouts[index].Get();

	Create(std::data(vkSetLayouts), static_cast<std::uint32_t>(layoutCount));
}

void PipelineLayout::Create(const DescriptorSetLayout& setLayout)
{
	VkDescriptorSetLayout vkSetLayout[] = { setLayout.Get() };
	Create(vkSetLayout, 1u);
}
