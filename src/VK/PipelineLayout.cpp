#include <PipelineLayout.hpp>
#include <VKThrowMacros.hpp>

PipelineLayout::PipelineLayout(VkDevice device)
	: m_deviceRef(device), m_createInfo{} {

	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_createInfo.setLayoutCount = 0u;
	m_createInfo.pSetLayouts = nullptr;
	m_createInfo.pushConstantRangeCount = 0u;
	m_createInfo.pPushConstantRanges = nullptr;
}

PipelineLayout::~PipelineLayout() noexcept {
	vkDestroyPipelineLayout(m_deviceRef, m_pipelineLayout, nullptr);
}

void PipelineLayout::CreateLayout() {
	VkResult result;
	VK_THROW_FAILED(result,
		vkCreatePipelineLayout(m_deviceRef, &m_createInfo, nullptr, &m_pipelineLayout)
	);
}

VkPipelineLayout PipelineLayout::GetLayout() const noexcept {
	return m_pipelineLayout;
}
