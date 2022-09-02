#include <Shader.hpp>
#include <PipelineLayout.hpp>
#include <VertexLayout.hpp>

#include <ModelContainer.hpp>
#include <PipelineObjectGFX.hpp>
#include <Terra.hpp>

ModelContainer::ModelContainer(
	std::string shaderPath, VkDevice device
) noexcept
	: m_pPerFrameBuffers{ device }, m_shaderPath{ std::move(shaderPath) } {}

void ModelContainer::AddModels(std::vector<std::shared_ptr<IModel>>&& models) {
	m_renderPipeline.AddOpaqueModels(std::move(models));
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
	m_renderPipeline.BindGraphicsPipeline(
		commandBuffer, Terra::descriptorSet->GetDescriptorSet()
	);

	m_pPerFrameBuffers.BindPerFrameBuffers(commandBuffer);

	m_renderPipeline.DrawModels(commandBuffer);
}

void ModelContainer::ReleaseUploadBuffers() {
	Terra::vertexBuffer->ReleaseUploadBuffer();
	Terra::indexBuffer->ReleaseUploadBuffer();
}

void ModelContainer::BindMemories(VkDevice device) {
	Terra::vertexBuffer->BindMemories(device);
	Terra::indexBuffer->BindMemories(device);
	Terra::uniformBuffer->BindMemories(device);
}

void ModelContainer::InitPipelines(VkDevice device, VkDescriptorSetLayout setLayout) {
	auto [pso, pipelineLayout] = CreatePipeline(device, setLayout);

	m_renderPipeline.AddGraphicsPipelineLayout(std::move(pipelineLayout));
	m_renderPipeline.AddGraphicsPipelineObject(std::move(pso));
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

void ModelContainer::AddModelInputs(
	VkDevice device,
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	m_pPerFrameBuffers.AddModelInputs(
		device, std::move(vertices), vertexBufferSize, std::move(indices), indexBufferSize
	);
}
