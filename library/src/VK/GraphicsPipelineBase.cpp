#include <GraphicsPipelineBase.hpp>

void GraphicsPipelineBase::Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
{
	VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->Get());
}
