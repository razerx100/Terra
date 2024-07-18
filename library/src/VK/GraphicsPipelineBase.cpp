#include <GraphicsPipelineBase.hpp>

void GraphicsPipelineBase::Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
{
	VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->Get());
}

void GraphicsPipelineBase::ManageFragmentShaderExtension(const std::wstring& fragmentShader) noexcept
{
	m_fragmentShader = fragmentShader;

	// Should check if the fragmentShader has the .spv extension. If not, then add it.
	if (!fragmentShader.ends_with(L".spv"))
		m_fragmentShader.append(L".spv");
}
