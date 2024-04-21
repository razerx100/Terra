#include <GraphicsPipelineBase.hpp>

void GraphicsPipelineBase::Bind(VkCommandBuffer graphicsCmdBuffer) const noexcept
{
	vkCmdBindPipeline(graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->Get());
}
