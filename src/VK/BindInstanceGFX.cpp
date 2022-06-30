#include <BindInstanceGFX.hpp>
#include <Terra.hpp>

BindInstanceGFX::BindInstanceGFX(
	std::unique_ptr<PipelineObjectGFX> pso, std::shared_ptr<PipelineLayout> layout
) noexcept : m_pipelineLayout(std::move(layout)), m_pso(std::move(pso)) {}

void BindInstanceGFX::AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddPipelineLayout(
	std::shared_ptr<PipelineLayout> layout
) noexcept {
	m_pipelineLayout = layout;

	for (const auto& modelRaw : m_modelsRaw)
		modelRaw->AddPipelineLayout(layout);
}

void BindInstanceGFX::AddModel(VkDevice device, std::shared_ptr<IModel>&& model) noexcept {
	std::shared_ptr<GpuBuffer> vertexBuffer = Terra::vertexBuffer->AddBuffer(
			device, model->GetVertexData(), model->GetVertexBufferSize()
		);

	std::shared_ptr<GpuBuffer> indexBuffer = Terra::indexBuffer->AddBuffer(
		device, model->GetIndexData(), model->GetIndexBufferSize()
	);

	size_t indexCount = model->GetIndexCount();

	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			device, std::move(vertexBuffer), std::move(indexBuffer), indexCount,
			std::move(model)
			)
	);
}

void BindInstanceGFX::DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept {
	for (const auto& model : m_modelsRaw)
		model->Draw(graphicsCmdBuffer);
}

void BindInstanceGFX::BindPipeline(
	VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
) const noexcept {
	vkCmdBindPipeline(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pso->GetPipelineObject()
	);

	VkDescriptorSet descSets[] = { descriptorSet };

	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw(VkDevice device, std::shared_ptr<IModel>&& model) noexcept
	: m_deviceRef(device), m_model(std::move(model)),
	m_vertexBuffer(VK_NULL_HANDLE), m_indexBuffer(VK_NULL_HANDLE),
	m_vertexOffset(0u), m_indexCount(0u) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	VkDevice device,
	std::shared_ptr<GpuBuffer> vertexBuffer,
	std::shared_ptr<GpuBuffer> indexBuffer,
	size_t indexCount,
	std::shared_ptr<IModel>&& model
) noexcept
	: m_deviceRef(device), m_model(std::move(model)),
	m_vertexBuffer(std::move(vertexBuffer)), m_indexBuffer(std::move(indexBuffer)),
	m_vertexOffset(0u), m_indexCount(static_cast<std::uint32_t>(indexCount)) {}

void BindInstanceGFX::ModelRaw::AddVertexBuffer(
	std::shared_ptr<GpuBuffer> buffer
) noexcept {
	m_vertexBuffer = std::move(buffer);
}

void BindInstanceGFX::ModelRaw::AddIndexBuffer(
	std::shared_ptr<GpuBuffer> buffer, size_t indexCount
) noexcept {
	m_indexBuffer = std::move(buffer);
	m_indexCount = static_cast<std::uint32_t>(indexCount);
}

void BindInstanceGFX::ModelRaw::AddPipelineLayout(
	std::shared_ptr<PipelineLayout> pipelineLayout
) noexcept {
	m_pPipelineLayout = std::move(pipelineLayout);
}

void BindInstanceGFX::ModelRaw::Draw(VkCommandBuffer commandBuffer) const noexcept {
	VkBuffer vertexBuffers[] = { m_vertexBuffer->GetBuffer() };

	vkCmdBindVertexBuffers(
		commandBuffer, 0u, 1u, vertexBuffers,
		&m_vertexOffset
	);
	vkCmdBindIndexBuffer(
		commandBuffer, m_indexBuffer->GetBuffer(), 0u,
		VK_INDEX_TYPE_UINT16
	);

	const std::uint32_t textureIndex = m_model->GetTextureIndex();
	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT,
		72u, 4u, &textureIndex
	);

	TextureOffset texOffset = m_model->GetTextureOffset();
	DirectX::XMMATRIX modelMat = m_model->GetModelMatrix();

	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT,
		0u, 64u, &modelMat
	);

	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT,
		64u, 8u, &texOffset
	);

	vkCmdDrawIndexed(commandBuffer, m_indexCount, 1u, 0u, 0u, 0u);
}
