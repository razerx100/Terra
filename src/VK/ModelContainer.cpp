#include <Shader.hpp>
#include <PipelineLayout.hpp>
#include <VertexLayout.hpp>

#include <ModelContainer.hpp>
#include <PipelineObjectGFX.hpp>
#include <Terra.hpp>

ModelContainer::ModelContainer(
	std::string shaderPath, VkDevice device
) noexcept
	: m_bindInstance(std::make_unique<BindInstancePerVertex>()),
	m_pPerFrameBuffers(std::make_unique<PerFrameBuffers>(device)),
	m_shaderPath(std::move(shaderPath)) {}

void ModelContainer::AddModels(
	VkDevice device, std::vector<std::shared_ptr<IModel>>&& models,
	std::unique_ptr<IModelInputs> modelInputs
) {
	m_bindInstance->AddModels(device, std::move(models), std::move(modelInputs));
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
	m_bindInstance->BindPipeline(
		commandBuffer,
		Terra::descriptorSet->GetDescriptorSet()
	);

	m_pPerFrameBuffers->UpdatePerFrameBuffers();

	m_bindInstance->DrawModels(commandBuffer);
}

void ModelContainer::ReleaseUploadBuffers() {
	Terra::vertexBuffer->ReleaseUploadBuffer();
	Terra::indexBuffer->ReleaseUploadBuffer();
}

void ModelContainer::CreateBuffers(VkDevice device) {
	Terra::vertexBuffer->CreateBuffers(device);
	Terra::indexBuffer->CreateBuffers(device);
	Terra::uniformBuffer->CreateBuffers(device);
}

void ModelContainer::InitPipelines(VkDevice device, VkDescriptorSetLayout setLayout) {
	auto [pso, pipelineLayout] = CreatePipeline(device, setLayout);

	m_bindInstance->AddPipelineLayout(std::move(pipelineLayout));

	m_bindInstance->AddPSO(std::move(pso));
}

ModelContainer::Pipeline ModelContainer::CreatePipeline(
	VkDevice device, VkDescriptorSetLayout setLayout
) const {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages
	pipelineLayout->AddPushConstantRange(
		VK_SHADER_STAGE_VERTEX_BIT,
		80u
	);
	pipelineLayout->AddPushConstantRange(
		VK_SHADER_STAGE_FRAGMENT_BIT,
		4u
	);

	pipelineLayout->CreateLayout(setLayout);

	auto vs = std::make_unique<Shader>(device);
	vs->CreateShader(device, m_shaderPath + "VertexShader.spv");

	auto fs = std::make_unique<Shader>(device);
	fs->CreateShader(device, m_shaderPath + "FragmentShader.spv");

	VertexLayout vertexLayout{};
	vertexLayout.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u);
	vertexLayout.AddInput(VK_FORMAT_R32G32_SFLOAT, 8u);
	vertexLayout.InitLayout();

	auto pso = std::make_unique<PipelineObjectGFX>(
		device,
		pipelineLayout->GetLayout(),
		Terra::renderPass->GetRenderPass(),
		vertexLayout.GetInputInfo(),
		vs->GetByteCode(),
		fs->GetByteCode()
		);

	return { std::move(pso), std::move(pipelineLayout) };
}
