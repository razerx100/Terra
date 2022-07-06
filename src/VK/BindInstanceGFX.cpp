#include <BindInstanceGFX.hpp>
#include <Terra.hpp>

// Bind Instance Base
BindInstanceGFX::BindInstanceGFX(
	std::unique_ptr<PipelineObjectGFX> pso, std::unique_ptr<PipelineLayout> layout
) noexcept : m_pipelineLayout(std::move(layout)), m_pso(std::move(pso)) {}

void BindInstanceGFX::AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept {
	m_pso = std::move(pso);
}

void BindInstanceGFX::AddPipelineLayout(
	std::unique_ptr<PipelineLayout> layout
) noexcept {
	m_pipelineLayout = std::move(layout);

	VkPipelineLayout pipelineLayout = m_pipelineLayout->GetLayout();

	for (const auto& model : m_models)
		model->AddPipelineLayout(pipelineLayout);
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

// Bind Instance Per Model Vertex
void BindInstancePerVertex::AddModels(
	VkDevice device, std::vector<std::shared_ptr<IModel>>&& models,
	std::unique_ptr<IModelInputs> modelInputs
) noexcept {
	std::shared_ptr<GpuBuffer> vertexBuffer = Terra::vertexBuffer->AddBuffer(
		device, modelInputs->GetVertexData(), modelInputs->GetVertexBufferSize()
	);

	std::shared_ptr<GpuBuffer> indexBuffer = Terra::indexBuffer->AddBuffer(
		device, modelInputs->GetIndexData(), modelInputs->GetIndexBufferSize()
	);

	auto modelSet = std::make_unique<ModelSetPerVertex>(
		std::move(vertexBuffer), std::move(indexBuffer)
		);

	for (auto& model : models)
		modelSet->AddInstance(std::move(model));

	m_models.emplace_back(std::move(modelSet));
}

void BindInstancePerVertex::DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept {
	for (const auto& model : m_models) {
		model->BindInputs(graphicsCmdBuffer);
		model->DrawInstances(graphicsCmdBuffer);
	}
}

// Bind Instance Global Vertex
// This won't work. Placeholder for now
void BindInstanceGVertex::AddModels(
	VkDevice device, std::vector<std::shared_ptr<IModel>>&& models,
	std::unique_ptr<IModelInputs> modelInputs
) noexcept {
	m_vertexBuffer = Terra::vertexBuffer->AddBuffer(
		device, modelInputs->GetVertexData(), modelInputs->GetVertexBufferSize()
	);

	std::shared_ptr<GpuBuffer> indexBuffer = Terra::indexBuffer->AddBuffer(
		device, modelInputs->GetIndexData(), modelInputs->GetIndexBufferSize()
	);

	auto modelSet = std::make_unique<ModelSetGVertex>(std::move(indexBuffer));

	for (auto& model : models)
		modelSet->AddInstance(std::move(model));

	m_models.emplace_back(std::move(modelSet));
}

void BindInstanceGVertex::DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept {
	VkBuffer vertexBuffers[] = { m_vertexBuffer->GetBuffer() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(
		graphicsCmdBuffer, 0u, 1u, vertexBuffers, vertexOffsets
	);

	for (const auto& model : m_models) {
		model->BindInputs(graphicsCmdBuffer);
		model->DrawInstances(graphicsCmdBuffer);
	}
}
