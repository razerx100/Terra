#include <ModelContainer.hpp>
#include <InstanceManager.hpp>
#include <VenusInstance.hpp>
#include <BindInstanceGFX.hpp>
#include <Shader.hpp>
#include <PipelineLayout.hpp>
#include <PipelineObjectGFX.hpp>

ModelContainer::ModelContainer(const char* shaderPath) noexcept
	: m_bindInstance(std::make_unique<BindInstanceGFX>()), m_shaderPath(shaderPath) {}

void ModelContainer::AddModel(
	VkDevice device, const IModel* const modelRef
) {
	m_bindInstance->AddModel(device, modelRef);
}

void ModelContainer::CopyData(std::atomic_size_t& workCount) {
	workCount += 2;

	GetVenusInstance()->SubmitWork(
		[&workCount] {
			VertexBufferInst::GetRef()->CopyData();

			--workCount;
		}
	);

	GetVenusInstance()->SubmitWork(
		[&workCount] {
			IndexBufferInst::GetRef()->CopyData();

			--workCount;
		}
	);
}

void ModelContainer::RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer) {
	VertexBufferInst::GetRef()->RecordUpload(device, copyBuffer);
	IndexBufferInst::GetRef()->RecordUpload(device, copyBuffer);
}

void ModelContainer::BindCommands(VkCommandBuffer commandBuffer) noexcept {
	m_bindInstance->BindCommands(commandBuffer);
}

void ModelContainer::ReleaseUploadBuffers() {
	VertexBufferInst::GetRef()->ReleaseUploadBuffer();
	IndexBufferInst::GetRef()->ReleaseUploadBuffer();
}

void ModelContainer::CreateBuffers(VkDevice device) {
	VertexBufferInst::GetRef()->CreateBuffer(device);
	IndexBufferInst::GetRef()->CreateBuffer(device);
}

void ModelContainer::InitPipelines(VkDevice device) {
	auto [pso, pipelineLayout] = CreatePipeline(
		device,
		m_bindInstance->GetVertexLayout()
	);

	m_bindInstance->AddPipelineLayout(pipelineLayout);

	m_bindInstance->AddPSO(std::move(pso));
}

ModelContainer::Pipeline ModelContainer::CreatePipeline(
	VkDevice device, const VertexLayout& layout
) const {
	std::shared_ptr<PipelineLayout> pipelineLayout =
		std::make_shared<PipelineLayout>(device);

	// Push constants needs to be serialized according to the shader stages
	pipelineLayout->AddPushConstantRange(
		VK_SHADER_STAGE_VERTEX_BIT,
		24u
	);
	pipelineLayout->AddPushConstantRange(
		VK_SHADER_STAGE_FRAGMENT_BIT,
		4u
	);

	pipelineLayout->CreateLayout();

	std::unique_ptr<Shader> vs = std::make_unique<Shader>(device);
	vs->CreateShader(device, m_shaderPath + "VertexShader.spv");

	std::unique_ptr<Shader> fs = std::make_unique<Shader>(device);
	fs->CreateShader(device, m_shaderPath + "FragmentShader.spv");

	std::unique_ptr<PipelineObjectGFX> pso = std::make_unique<PipelineObjectGFX>(
		device,
		pipelineLayout->GetLayout(),
		RndrPassInst::GetRef()->GetRenderPass(),
		layout.GetInputInfo(),
		vs->GetByteCode(),
		fs->GetByteCode()
		);

	return {
		std::move(pso),
		pipelineLayout
	};
}
