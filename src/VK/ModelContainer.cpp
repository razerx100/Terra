#include <ModelContainer.hpp>
#include <InstanceManager.hpp>
#include <VenusInstance.hpp>
#include <BindInstanceGFX.hpp>
#include <Shader.hpp>
#include <PipelineLayout.hpp>
#include <PipelineObjectGFX.hpp>
#include <VertexLayout.hpp>

ModelContainer::ModelContainer(const char* shaderPath) noexcept
	: m_coloredInstanceData{}, m_texturedInstanceData{}, m_shaderPath(shaderPath) {}

void ModelContainer::AddModel(
	VkDevice device, const IModel* const modelRef,
	bool texture
) {
	if (texture)
		AddTexturedModel(device, modelRef);
	else
		AddColoredModel(device, modelRef);
}

void ModelContainer::CopyData() {
	std::atomic_size_t works = 2u;

	GetVenusInstance()->SubmitWork(
		[&works] {
			VertexBufferInst::GetRef()->CopyData();

			--works;
		}
	);

	GetVenusInstance()->SubmitWork(
		[&works] {
			IndexBufferInst::GetRef()->CopyData();

			--works;
		}
	);

	while (works != 0u);
}

void ModelContainer::RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer) {
	VertexBufferInst::GetRef()->RecordUpload(device, copyBuffer);
	IndexBufferInst::GetRef()->RecordUpload(device, copyBuffer);
}

void ModelContainer::BindCommands(VkCommandBuffer commandBuffer) noexcept {
	for (auto& bindInstance : m_bindInstances)
		bindInstance->BindCommands(commandBuffer);
}

void ModelContainer::InitNewInstance(InstanceData& instanceData, bool texure) noexcept {
	m_bindInstances.emplace_back(std::make_unique<BindInstanceGFX>(texure));
	instanceData = { true, m_bindInstances.size() - 1u };
}

void ModelContainer::AddColoredModel(VkDevice device, const IModel* const modelRef) {
	if (!m_coloredInstanceData.available) {
		InitNewInstance(m_coloredInstanceData, false);

		std::unique_ptr<PipelineLayout> pipelineLayout =
			std::make_unique<PipelineLayout>(device);

		pipelineLayout->CreateLayout();

		VertexLayout vertexLayout = modelRef->GetVertexLayout();

		std::unique_ptr<Shader> vs = std::make_unique<Shader>();
		vs->CreateShader(device, m_shaderPath + "VSColored.spv");

		std::unique_ptr<Shader> fs = std::make_unique<Shader>();
		vs->CreateShader(device, m_shaderPath + "FSColored.spv");

		std::unique_ptr<PipelineObjectGFX> pso = std::make_unique<PipelineObjectGFX>(
			device,
			pipelineLayout->GetLayout(),
			RndrPassInst::GetRef()->GetRenderPass(),
			vertexLayout.GetInputInfo(),
			vs->GetByteCode(),
			fs->GetByteCode()
			);

		m_bindInstances[m_coloredInstanceData.index]->AddPipelineLayout(
			std::move(pipelineLayout)
		);

		m_bindInstances[m_coloredInstanceData.index]->AddPSO(
			std::move(pso)
		);
	}

	m_bindInstances[m_coloredInstanceData.index]->AddModel(device, modelRef);
}

void ModelContainer::AddTexturedModel(VkDevice device, const IModel* const modelRef) {
	if (!m_texturedInstanceData.available) {
		InitNewInstance(m_texturedInstanceData, true);

		// Iniit Pipeline
	}

	m_bindInstances[m_texturedInstanceData.index]->AddModel(device, modelRef);
}

void ModelContainer::ReleaseUploadBuffers(VkDevice device) {
	VertexBufferInst::GetRef()->ReleaseUploadBuffer(device);
	IndexBufferInst::GetRef()->ReleaseUploadBuffer(device);
}
