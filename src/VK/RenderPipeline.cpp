#include <RenderPipeline.hpp>
#include <ranges>
#include <algorithm>

void RenderPipeline::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void RenderPipeline::AddGraphicsPipelineObject(
	std::unique_ptr<PipelineObjectGFX> pso
) noexcept {
	m_graphicsPSO = std::move(pso);
}

void RenderPipeline::AddGraphicsPipelineLayout(
	std::unique_ptr<PipelineLayout> layout
) noexcept {
	m_graphicsPipelineLayout = std::move(layout);
}

void RenderPipeline::BindGraphicsPipeline(
	VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
) const noexcept {
	vkCmdBindPipeline(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPSO->GetPipelineObject()
	);

	VkDescriptorSet descSets[] = { descriptorSet };

	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphicsPipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);
}

void RenderPipeline::DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept {
	for (auto& model : m_opaqueModels) {
		const std::uint32_t textureIndex = model->GetTextureIndex();
		vkCmdPushConstants(
			graphicsCmdBuffer, m_graphicsPipelineLayout->GetLayout(),
			VK_SHADER_STAGE_FRAGMENT_BIT, 80u, 4u, &textureIndex
		);

		const UVInfo uvInfo = model->GetUVInfo();
		const DirectX::XMMATRIX modelMat = model->GetModelMatrix();

		vkCmdPushConstants(
			graphicsCmdBuffer, m_graphicsPipelineLayout->GetLayout(),
			VK_SHADER_STAGE_VERTEX_BIT, 0u, 64u, &modelMat
		);

		vkCmdPushConstants(
			graphicsCmdBuffer, m_graphicsPipelineLayout->GetLayout(),
			VK_SHADER_STAGE_VERTEX_BIT, 64u, 16u, &uvInfo
		);

		const std::uint32_t indexCount = model->GetIndexCount();
		const std::uint32_t indexOffset = model->GetIndexOffset();
		vkCmdDrawIndexed(graphicsCmdBuffer, indexCount, 1u, indexOffset, 0u, 0u);
	}
}
