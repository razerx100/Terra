#include <ModelContainer.hpp>
#include <Shader.hpp>
#include <PipelineLayout.hpp>
#include <PipelineObjectGFX.hpp>
#include <Terra.hpp>

ModelContainer::ModelContainer(std::string shaderPath) noexcept
	:
	m_bindInstance(std::make_unique<BindInstanceGFX>()),
	m_shaderPath(std::move(shaderPath)) {}

void ModelContainer::AddModel(
	VkDevice device, const IModel* const modelRef
) {
	m_bindInstance->AddModel(device, modelRef);
}

void ModelContainer::CopyData(std::atomic_size_t& workCount) {
	workCount += 2;

	Terra::threadPool->SubmitWork(
		[&workCount] {
			Terra::vertexBuffer->CopyData();

			--workCount;
		}
	);

	Terra::threadPool->SubmitWork(
		[&workCount] {
			Terra::indexBuffer->CopyData();

			--workCount;
		}
	);
}

void ModelContainer::RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer) {
	Terra::vertexBuffer->RecordUpload(device, copyBuffer);
	Terra::indexBuffer->RecordUpload(device, copyBuffer);
}

void ModelContainer::BindCommands(VkCommandBuffer commandBuffer) const noexcept {
	m_bindInstance->BindCommands(
		commandBuffer,
		Terra::descriptorSet->GetDescriptorSet()
	);
}

void ModelContainer::ReleaseUploadBuffers() {
	Terra::vertexBuffer->ReleaseUploadBuffer();
	Terra::indexBuffer->ReleaseUploadBuffer();
}

void ModelContainer::CreateBuffers(VkDevice device) {
	Terra::vertexBuffer->CreateBuffer(device);
	Terra::indexBuffer->CreateBuffer(device);
}

void ModelContainer::InitPipelines(
	VkDevice device,
	const std::vector<VkDescriptorSetLayout>& setLayout
) {
	auto [pso, pipelineLayout] = CreatePipeline(
		device,
		m_bindInstance->GetVertexLayout(),
		setLayout
	);

	m_bindInstance->AddPipelineLayout(pipelineLayout);

	m_bindInstance->AddPSO(std::move(pso));
}

ModelContainer::Pipeline ModelContainer::CreatePipeline(
	VkDevice device, const VertexLayout& layout,
	const std::vector<VkDescriptorSetLayout>& setLayout
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

	pipelineLayout->CreateLayout(setLayout);

	std::unique_ptr<Shader> vs = std::make_unique<Shader>(device);
	vs->CreateShader(device, m_shaderPath + "VertexShader.spv");

	std::unique_ptr<Shader> fs = std::make_unique<Shader>(device);
	fs->CreateShader(device, m_shaderPath + "FragmentShader.spv");

	std::unique_ptr<PipelineObjectGFX> pso = std::make_unique<PipelineObjectGFX>(
		device,
		pipelineLayout->GetLayout(),
		Terra::renderPass->GetRenderPass(),
		layout.GetInputInfo(),
		vs->GetByteCode(),
		fs->GetByteCode()
		);

	return {
		std::move(pso),
		pipelineLayout
	};
}
