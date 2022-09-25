#include <Shader.hpp>
#include <PipelineLayout.hpp>
#include <VertexLayout.hpp>

#include <ModelManager.hpp>
#include <PipelineObjectGFX.hpp>
#include <Terra.hpp>

ModelManager::ModelManager(
	VkDevice device, std::vector<std::uint32_t> queueFamilyIndices,
	std::uint32_t bufferCount
) noexcept
	: m_renderPipeline{ device, queueFamilyIndices },
	m_perFrameBuffers{ device, std::move(queueFamilyIndices), bufferCount } {}

void ModelManager::SetShaderPath(std::wstring path) noexcept {
	m_shaderPath = std::move(path);
}

void ModelManager::AddModels(std::vector<std::shared_ptr<IModel>>&& models) {
	m_renderPipeline.AddOpaqueModels(std::move(models));
}

void ModelManager::RecordUploadBuffers(VkCommandBuffer copyBuffer) noexcept {
	m_perFrameBuffers.RecordCopy(copyBuffer);
	m_renderPipeline.RecordCopy(copyBuffer);
}

void ModelManager::BindCommands(
	VkCommandBuffer commandBuffer, size_t frameIndex
) const noexcept {
	m_renderPipeline.BindGraphicsPipeline(
		commandBuffer, Terra::descriptorSet->GetDescriptorSet(frameIndex)
	);

	const auto vkFrameIndex = static_cast<VkDeviceSize>(frameIndex);

	m_perFrameBuffers.BindPerFrameBuffers(commandBuffer, vkFrameIndex);
	m_renderPipeline.UpdateModelData(vkFrameIndex);
	m_renderPipeline.DrawModels(commandBuffer, vkFrameIndex);
}

void ModelManager::ReleaseUploadBuffers() {
	m_perFrameBuffers.ReleaseUploadResources();
	m_renderPipeline.ReleaseUploadResources();
}

void ModelManager::BindMemories(VkDevice device) {
	m_perFrameBuffers.BindResourceToMemory(device);
	m_renderPipeline.BindResourceToMemory(device);
}

void ModelManager::InitPipelines(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) {
	auto [pso, pipelineLayout] = CreatePipeline(device, layoutCount, setLayouts);

	m_renderPipeline.AddGraphicsPipelineLayout(std::move(pipelineLayout));
	m_renderPipeline.AddGraphicsPipelineObject(std::move(pso));
}

ModelManager::Pipeline ModelManager::CreatePipeline(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) const {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

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
	m_perFrameBuffers.AddModelInputs(
		device, std::move(vertices), vertexBufferSize, std::move(indices), indexBufferSize
	);
}

void ModelManager::CreateBuffers(VkDevice device, std::uint32_t bufferCount) noexcept {
	m_renderPipeline.CreateBuffers(device, bufferCount);
}

void ModelManager::CopyData() noexcept {
	m_renderPipeline.CopyData();
}
