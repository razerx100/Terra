#include <ModelSet.hpp>

// Model Instance
ModelInstance::ModelInstance(std::shared_ptr<IModel>&& model) noexcept
	: m_model{ std::move(model) }, m_pPipelineLayout{ VK_NULL_HANDLE } {}

void ModelInstance::AddPipelineLayout(VkPipelineLayout pipelineLayout) noexcept {
	m_pPipelineLayout = pipelineLayout;
}

void ModelInstance::Draw(VkCommandBuffer commandBuffer) const noexcept {
	const std::uint32_t textureIndex = m_model->GetTextureIndex();
	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
		80u, 4u, &textureIndex
	);

	const UVInfo uvInfo = m_model->GetUVInfo();
	const DirectX::XMMATRIX modelMat = m_model->GetModelMatrix();

	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
		0u, 64u, &modelMat
	);

	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
		64u, 16u, &uvInfo
	);

	const std::uint32_t indexCount = m_model->GetIndexCount();
	vkCmdDrawIndexed(commandBuffer, indexCount, 1u, 0u, 0u, 0u);
}

// Model Set Vertex
void ModelSetVertex::AddInstance(std::shared_ptr<IModel>&& model) noexcept {
	m_modelInstances.emplace_back(std::move(model));
}

void ModelSetVertex::AddPipelineLayout(VkPipelineLayout pipelineLayout) noexcept {
	for (auto& instance : m_modelInstances)
		instance.AddPipelineLayout(pipelineLayout);
}

void ModelSetVertex::DrawInstances(VkCommandBuffer commandBuffer) const noexcept {
	for (const auto& instance : m_modelInstances)
		instance.Draw(commandBuffer);
}

// Model Set Per Vertex
ModelSetPerVertex::ModelSetPerVertex(
	std::shared_ptr<GpuBuffer> vertexBuffer, std::shared_ptr<GpuBuffer> indexBuffer
) noexcept
	: m_vertexBuffer{ std::move(vertexBuffer) }, m_indexBuffer{ std::move(indexBuffer) } {}

void ModelSetPerVertex::BindInputs(VkCommandBuffer commandBuffer) const noexcept {
	VkBuffer vertexBuffers[] = { m_vertexBuffer->GetBuffer() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(
		commandBuffer, 0u, 1u, vertexBuffers, vertexOffsets
	);
	vkCmdBindIndexBuffer(
		commandBuffer, m_indexBuffer->GetBuffer(), 0u,
		VK_INDEX_TYPE_UINT16
	);
}

// Model Set Global Vertex
ModelSetGVertex::ModelSetGVertex(std::shared_ptr<GpuBuffer> indexBuffer) noexcept
	: m_indexBuffer{ std::move(indexBuffer) } {}

void ModelSetGVertex::BindInputs(VkCommandBuffer commandBuffer) const noexcept {
	vkCmdBindIndexBuffer(
		commandBuffer, m_indexBuffer->GetBuffer(), 0u,
		VK_INDEX_TYPE_UINT16
	);
}
