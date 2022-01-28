#include <BindInstanceGFX.hpp>
#include <InstanceManager.hpp>

BindInstanceGFX::BindInstanceGFX(bool textureAvailable) noexcept
	: m_textureAvailable(textureAvailable) {}

BindInstanceGFX::BindInstanceGFX(
	bool textureAvailable,
	std::unique_ptr<IPipelineObject> pso,
	std::unique_ptr<IPipelineLayout> layout
) noexcept
	: m_textureAvailable(textureAvailable), m_pso(std::move(pso)),
	m_pipelineLayout(std::move(layout)) {}

void BindInstanceGFX::AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddPipelineLayout(
	std::unique_ptr<IPipelineLayout> layout
) noexcept {
	m_pipelineLayout = std::move(layout);
}

void BindInstanceGFX::AddModel(
	VkDevice device, const IModel* const modelRef
) noexcept {
	m_modelsRaw.emplace_back(
		std::make_unique<ModelRaw>(
			device,
			VertexBufferInst::GetRef()->AddBuffer(
				device, modelRef->GetVertexData(), modelRef->GetVertexBufferSize()
			),
			IndexBufferInst::GetRef()->AddBuffer(
				device, modelRef->GetIndexData(), modelRef->GetIndexBufferSize()
			),
			modelRef->GetIndexCount()
			)
	);
}

void BindInstanceGFX::BindCommands(VkCommandBuffer commandBuffer) noexcept {
	vkCmdBindPipeline(
		commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pso->GetPipelineObject()
	);

	for (auto& model : m_modelsRaw)
		model->Draw(commandBuffer);
}

// Model Raw
BindInstanceGFX::ModelRaw::ModelRaw(VkDevice device) noexcept
	: m_deviceRef(device), m_vertexBuffer(nullptr), m_indexBuffer(nullptr),
	m_vertexOffset(0u), m_indexCount(0u) {}

BindInstanceGFX::ModelRaw::ModelRaw(
	VkDevice device,
	VkBuffer vertexBuffer,
	VkBuffer indexBuffer,
	size_t indicesCount
) noexcept
	: m_deviceRef(device), m_vertexBuffer(vertexBuffer), m_indexBuffer(indexBuffer),
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

void BindInstanceGFX::ModelRaw::Draw(VkCommandBuffer commandBuffer) noexcept {
	vkCmdBindVertexBuffers(commandBuffer, 0u, 1u, &m_vertexBuffer, &m_vertexOffset);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0u, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(commandBuffer, m_indexCount, 1u, 0u, 0u, 0u);
}

