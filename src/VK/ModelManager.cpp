#include <ModelManager.hpp>

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

// Model Manager Vertex
void ModelManagerVertex::AddInstance(std::shared_ptr<IModel>&& model) noexcept {
	m_modelInstances.emplace_back(std::move(model));
}

void ModelManagerVertex::AddPipelineLayout(VkPipelineLayout pipelineLayout) noexcept {
	for (auto& instance : m_modelInstances)
		instance.AddPipelineLayout(pipelineLayout);
}

void ModelManagerVertex::BindInstances(VkCommandBuffer commandBuffer) const noexcept {
	for (auto& instance : m_modelInstances)
		instance.Draw(commandBuffer);
}

// Model Manager Per Vertex
ModelManagerPerVertex::ModelManagerPerVertex(
	std::shared_ptr<GpuBuffer> vertexBuffer, std::shared_ptr<GpuBuffer> indexBuffer
) noexcept
	: m_vertexBuffer{ std::move(vertexBuffer) }, m_indexBuffer{ std::move(indexBuffer) } {}

void ModelManagerPerVertex::BindInputs(VkCommandBuffer commandBuffer) const noexcept {
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

// Model Manager Global Vertex
ModelManagerGVertex::ModelManagerGVertex(std::shared_ptr<GpuBuffer> indexBuffer) noexcept
	: m_indexBuffer{ std::move(indexBuffer) } {}

void ModelManagerGVertex::BindInputs(VkCommandBuffer commandBuffer) const noexcept {
	vkCmdBindIndexBuffer(
		commandBuffer, m_indexBuffer->GetBuffer(), 0u,
		VK_INDEX_TYPE_UINT16
	);
}
