#include <BindInstanceGFX.hpp>
#include <InstanceManager.hpp>

BindInstanceGFX::BindInstanceGFX() noexcept
	: m_vertexLayoutAvailable(false) {}

BindInstanceGFX::BindInstanceGFX(
	std::unique_ptr<IPipelineObject> pso,
	std::shared_ptr<IPipelineLayout> layout
) noexcept
	: m_pso(std::move(pso)),
	m_pipelineLayout(layout),
	m_vertexLayoutAvailable(false) {}

void BindInstanceGFX::AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddPipelineLayout(
	std::shared_ptr<IPipelineLayout> layout
) noexcept {
	m_pipelineLayout = layout;

	for (auto& modelRaw : m_modelsRaw)
		modelRaw->AddPipelineLayout(layout);
}

void BindInstanceGFX::AddModel(
	VkDevice device, const IModel* const modelRef
) noexcept {
	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			device,
			modelRef,
			VertexBufferInst::GetRef()->AddBuffer(
				device, modelRef->GetVertexData(), modelRef->GetVertexBufferSize()
			),
			IndexBufferInst::GetRef()->AddBuffer(
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

	for (auto& model : m_modelsRaw)
		model->Draw(commandBuffer);
}

VertexLayout BindInstanceGFX::GetVertexLayout() const noexcept {
	return m_vertexLayout;
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw(VkDevice device, const IModel* const modelRef) noexcept
	: m_deviceRef(device), m_modelRef(modelRef),
	m_vertexBuffer(nullptr), m_indexBuffer(nullptr),
	m_vertexOffset(0u), m_indexCount(0u) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	VkDevice device,
	const IModel* const modelRef,
	VkBuffer vertexBuffer,
	VkBuffer indexBuffer,
	size_t indicesCount
) noexcept
	: m_deviceRef(device),m_modelRef(modelRef),
	m_vertexBuffer(vertexBuffer), m_indexBuffer(indexBuffer),
	m_vertexOffset(0u), m_indexCount(static_cast<std::uint32_t>(indicesCount)) {}

BindInstanceGFX::ModelRaw::~ModelRaw() noexcept {
	vkDestroyBuffer(m_deviceRef, m_vertexBuffer, nullptr);
	vkDestroyBuffer(m_deviceRef, m_indexBuffer, nullptr);
}

void BindInstanceGFX::ModelRaw::AddVertexBuffer(VkBuffer buffer) noexcept {
	m_vertexBuffer = buffer;
}

void BindInstanceGFX::ModelRaw::AddIndexBuffer(
	VkBuffer buffer, size_t indexCount
) noexcept {
	m_indexBuffer = buffer;
	m_indexCount = static_cast<std::uint32_t>(indexCount);
}

void BindInstanceGFX::ModelRaw::AddPipelineLayout(
	std::shared_ptr<IPipelineLayout> pipelineLayout
) noexcept {
	m_pPipelineLayout = pipelineLayout;
}

void BindInstanceGFX::ModelRaw::Draw(VkCommandBuffer commandBuffer) noexcept {
	vkCmdBindVertexBuffers(commandBuffer, 0u, 1u, &m_vertexBuffer, &m_vertexOffset);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0u, VK_INDEX_TYPE_UINT16);

	std::uint32_t textureIndex = m_modelRef->GetTextureIndex();
	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT,
		0u, 4u, &textureIndex
	);

	const TextureData& texInfo = m_modelRef->GetTextureInfo();
	vkCmdPushConstants(
		commandBuffer, m_pPipelineLayout->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT,
		4u, 24u, &texInfo
	);

	vkCmdDrawIndexed(commandBuffer, m_indexCount, 1u, 0u, 0u, 0u);
}

