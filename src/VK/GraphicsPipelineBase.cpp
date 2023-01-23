#include <GraphicsPipelineBase.hpp>

void GraphicsPipelineBase::BindGraphicsPipeline(
	VkCommandBuffer graphicsCmdBuffer
) const noexcept {
	vkCmdBindPipeline(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->GetPipeline()
	);
}

void GraphicsPipelineBase::CreateGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath
) noexcept {
	m_graphicsPipeline = _createGraphicsPipeline(
		device, graphicsLayout, renderPass, shaderPath, m_fragmentShader
	);
}

