#include <BindInstanceGFX.hpp>
#include <Terra.hpp>

BindInstanceGFX::BindInstanceGFX() noexcept
	: m_vertexLayoutAvailable(false) {}

BindInstanceGFX::BindInstanceGFX(
	std::unique_ptr<PipelineObjectGFX> pso,
	std::shared_ptr<PipelineLayout> layout
) noexcept
	:
	m_pipelineLayout(std::move(layout)),
	m_pso(std::move(pso)),
	m_vertexLayoutAvailable(false) {}

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

void BindInstanceGFX::AddModel(
	VkDevice device, const IModel* const modelRef
) noexcept {
	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			device,
			modelRef,
			Terra::vertexBuffer->AddBuffer(
				device, modelRef->GetVertexData(), modelRef->GetVertexBufferSize()
			),
			Terra::indexBuffer->AddBuffer(
				device, modelRef->GetIndexData(), modelRef->GetIndexBufferSize()
			),
			modelRef->GetIndexCount()
			)
	);

	if (!m_vertexLayoutAvailable) {
		m_vertexLayout.InitLayout(modelRef->GetVertexLayout());
		m_vertexLayoutAvailable = true;
	}
}

void BindInstanceGFX::BindCommands(VkCommandBuffer commandBuffer) noexcept {
	vkCmdBindPipeline(
		commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pso->GetPipelineObject()
	);

	for (const auto& model : m_modelsRaw)
		model->Draw(commandBuffer);
}

VertexLayout BindInstanceGFX::GetVertexLayout() const noexcept {
	return m_vertexLayout;
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw(VkDevice device, const IModel* const modelRef) noexcept
	: m_deviceRef(device), m_modelRef(modelRef),
	m_vertexBuffer(VK_NULL_HANDLE), m_indexBuffer(VK_NULL_HANDLE),
	m_vertexOffset(0u), m_indexCount(0u) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	VkDevice device,
	const IModel* const modelRef,
	std::unique_ptr<GpuBuffer> vertexBuffer,
	std::unique_ptr<GpuBuffer> indexBuffer,
	size_t indexCount
) noexcept
	: m_deviceRef(device),m_modelRef(modelRef),
	m_vertexBuffer(std::move(vertexBuffer)), m_indexBuffer(std::move(indexBuffer)),
	m_vertexOffset(0u), m_indexCount(static_cast<std::uint32_t>(indexCount)) {}

void BindInstanceGFX::ModelRaw::AddVertexBuffer(
	std::unique_ptr<GpuBuffer> buffer
) noexcept {
	m_vertexBuffer = std::move(buffer);
}

void BindInstanceGFX::ModelRaw::AddIndexBuffer(
	std::unique_ptr<GpuBuffer> buffer, size_t indexCount
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

	const std::uint32_t textureIndex = m_modelRef->GetTextureIndex();
	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT,
		24u, 4u, &textureIndex
	);

	const TextureData& texInfo = m_modelRef->GetTextureInfo();
	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT,
		0u, 24u, &texInfo
	);

	vkCmdDrawIndexed(commandBuffer, m_indexCount, 1u, 0u, 0u, 0u);
}

