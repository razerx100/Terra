#include <PipelineLayout.hpp>
#include <VKThrowMacros.hpp>

PipelineLayout::PipelineLayout(VkDevice device)
	: m_deviceRef(device), m_pipelineLayout(VK_NULL_HANDLE), m_pushConstantOffset(0u) {}

PipelineLayout::~PipelineLayout() noexcept {
	vkDestroyPipelineLayout(m_deviceRef, m_pipelineLayout, nullptr);
}

void PipelineLayout::AddPushConstantRange(
	VkShaderStageFlags shaderFlag, std::uint32_t rangeSize
) noexcept {
	VkPushConstantRange range = {};
	range.stageFlags = shaderFlag;
	range.size = rangeSize;
	range.offset = m_pushConstantOffset;

	m_pushConstantOffset += rangeSize;

	m_pushRanges.emplace_back(range);
}

void PipelineLayout::CreateLayout() {
	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 0u;
	createInfo.pSetLayouts = nullptr;
	createInfo.pushConstantRangeCount = static_cast<std::uint32_t>(m_pushRanges.size());
	createInfo.pPushConstantRanges = m_pushRanges.data();

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreatePipelineLayout(m_deviceRef, &createInfo, nullptr, &m_pipelineLayout)
	);
}

VkPipelineLayout PipelineLayout::GetLayout() const noexcept {
	return m_pipelineLayout;
}
