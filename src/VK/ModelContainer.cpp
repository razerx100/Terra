#include <Shader.hpp>
#include <PipelineLayout.hpp>

#include <ModelContainer.hpp>
#include <PipelineObjectGFX.hpp>
#include <Terra.hpp>

ModelContainer::ModelContainer(
	std::string shaderPath, VkDevice device
) noexcept
	:
	m_bindInstance(std::make_unique<BindInstanceGFX>()),
	m_pPerFrameBuffers(std::make_unique<PerFrameBuffers>(device)),
	m_shaderPath(std::move(shaderPath)) {}

void ModelContainer::AddModel(VkDevice device, std::shared_ptr<IModel>&& model) {
	m_bindInstance->AddModel(device, std::move(model));
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

void ModelContainer::InitPipelines(
	VkDevice device,
	VkDescriptorSetLayout setLayout
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
	VkDescriptorSetLayout setLayout
) const {
	std::shared_ptr<PipelineLayout> pipelineLayout =
		std::make_shared<PipelineLayout>(device);

	// Push constants needs to be serialized according to the shader stages
	pipelineLayout->AddPushConstantRange(
		VK_SHADER_STAGE_VERTEX_BIT,
		72u
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
