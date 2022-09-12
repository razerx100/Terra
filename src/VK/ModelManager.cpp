#include <Shader.hpp>
#include <PipelineLayout.hpp>
#include <VertexLayout.hpp>

#include <ModelManager.hpp>
#include <PipelineObjectGFX.hpp>
#include <Terra.hpp>

ModelManager::ModelManager(
	VkDevice device, std::vector<std::uint32_t> queueFamilyIndices
) noexcept : m_pPerFrameBuffers{ device, std::move(queueFamilyIndices) } {}

void ModelManager::SetShaderPath(std::wstring path) noexcept {
	m_shaderPath = std::move(path);
}

void ModelManager::AddModels(std::vector<std::shared_ptr<IModel>>&& models) {
	m_renderPipeline.AddOpaqueModels(std::move(models));
}

void ModelManager::RecordUploadBuffers(VkCommandBuffer copyBuffer) {
	m_pPerFrameBuffers.RecordCopy(copyBuffer);
}

void ModelManager::BindCommands(VkCommandBuffer commandBuffer) const noexcept {
	m_renderPipeline.BindGraphicsPipeline(
		commandBuffer, Terra::descriptorSet->GetDescriptorSet()
	);

	m_pPerFrameBuffers.BindPerFrameBuffers(commandBuffer);

	m_renderPipeline.DrawModels(commandBuffer);
}

void ModelManager::ReleaseUploadBuffers() {
	m_pPerFrameBuffers.ReleaseUploadResources();
}

void ModelManager::BindMemories(VkDevice device) {
	VkDeviceMemory uploadMemory = Terra::Resources::uploadMemory->GetMemoryHandle();
	VkDeviceMemory cpuWriteMemory = Terra::Resources::cpuWriteMemory->GetMemoryHandle();
	VkDeviceMemory gpuMemory = Terra::Resources::gpuOnlyMemory->GetMemoryHandle();

	m_pPerFrameBuffers.BindResourceToMemory(device, uploadMemory, cpuWriteMemory, gpuMemory);
}

void ModelManager::InitPipelines(VkDevice device, VkDescriptorSetLayout setLayout) {
	auto [pso, pipelineLayout] = CreatePipeline(device, setLayout);

	m_renderPipeline.AddGraphicsPipelineLayout(std::move(pipelineLayout));
	m_renderPipeline.AddGraphicsPipelineObject(std::move(pso));
}

ModelManager::Pipeline ModelManager::CreatePipeline(
	VkDevice device, VkDescriptorSetLayout setLayout
) const {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages
	pipelineLayout->AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 80u);
	pipelineLayout->AddPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 4u);

	pipelineLayout->CreateLayout(setLayout);

	auto vs = std::make_unique<Shader>(device);
	vs->CreateShader(device, m_shaderPath + L"VertexShader.spv");

	auto fs = std::make_unique<Shader>(device);
	fs->CreateShader(device, m_shaderPath + L"FragmentShader.spv");

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

void ModelManager::AddModelInputs(
	VkDevice device,
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	m_pPerFrameBuffers.AddModelInputs(
		device, std::move(vertices), vertexBufferSize, std::move(indices), indexBufferSize
	);
}
