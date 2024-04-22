
#include <Terra.hpp>
#include <RenderEngineVertexShader.hpp>

// Vertex Shader
RenderEngineVertexShader::RenderEngineVertexShader(
	VkDevice device, QueueIndicesTG queueIndices
//) : m_vertexManager{ device }, m_queueIndices{ queueIndices } {}
) : m_queueIndices{ queueIndices } {}

void RenderEngineVertexShader::AddGVerticesAndIndices(
	VkDevice device, std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) noexcept {
	//m_vertexManager.AddGVerticesAndIndices(device, std::move(gVertices), std::move(gIndices));
}

void RenderEngineVertexShader::BindGraphicsBuffers(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	BindGraphicsDescriptorSets(graphicsCmdBuffer, frameIndex);
	//m_vertexManager.BindVertexAndIndexBuffer(graphicsCmdBuffer);
}

void RenderEngineVertexShader::BindResourcesToMemory(VkDevice device) {
	//m_vertexManager.BindResourceToMemory(device);
	_bindResourcesToMemory(device);
}

void RenderEngineVertexShader::RecordCopy(VkCommandBuffer transferBuffer) noexcept {
	//m_vertexManager.RecordCopy(transferBuffer);
	_recordCopy(transferBuffer);
}

void RenderEngineVertexShader::ReleaseUploadResources() noexcept {
	//m_vertexManager.ReleaseUploadResources();
	_releaseUploadResources();
}

void RenderEngineVertexShader::AcquireOwnerShipGraphics(
	VkCommandBuffer graphicsCmdBuffer
) noexcept {
	/*m_vertexManager.AcquireOwnerShips(
		graphicsCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics
	);*/
}

void RenderEngineVertexShader::ReleaseOwnership(
	VkCommandBuffer transferCmdBuffer
) noexcept {
	/*m_vertexManager.ReleaseOwnerships(
		transferCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics
	);*/
	_releaseOwnership(transferCmdBuffer);
}

std::unique_ptr<PipelineLayout> RenderEngineVertexShader::CreateGraphicsPipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) const noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}

void RenderEngineVertexShader::_bindResourcesToMemory(
	[[maybe_unused]] VkDevice device
) {}
void RenderEngineVertexShader::_recordCopy(
	[[maybe_unused]] VkCommandBuffer transferBuffer
) noexcept {}
void RenderEngineVertexShader::_releaseUploadResources() noexcept {}
void RenderEngineVertexShader::_releaseOwnership(
	[[maybe_unused]] VkCommandBuffer transferCmdBuffer
) noexcept {}

// Indirect Draw
RenderEngineIndirectDraw::RenderEngineIndirectDraw(
	VkDevice device, std::uint32_t bufferCount, QueueIndices3 queueIndices
) : RenderEngineVertexShader{ device, queueIndices } {}
	//m_computePipeline{ device, bufferCount, queueIndices } {}

void RenderEngineIndirectDraw::ExecutePreRenderStage(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	ExecuteComputeStage(frameIndex);
	ExecutePreGraphicsStage(graphicsCmdBuffer, frameIndex);
}

void RenderEngineIndirectDraw::RecordDrawCommands(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	// One Pipeline needs to be bound before Descriptors can be bound.
	//m_graphicsPipeline0->Bind(graphicsCmdBuffer);
	BindGraphicsBuffers(graphicsCmdBuffer, frameIndex);

	//VkBuffer argumentBuffer = m_computePipeline.GetArgumentBuffer(frameIndex);
	//VkBuffer counterBuffer = m_computePipeline.GetCounterBuffer(frameIndex);

	//m_graphicsPipeline0->DrawModels(graphicsCmdBuffer, argumentBuffer, counterBuffer);

	for (auto& graphicsPipeline : m_graphicsPipelines) {
		//graphicsPipeline->Bind(graphicsCmdBuffer);
		//graphicsPipeline->DrawModels(graphicsCmdBuffer, argumentBuffer, counterBuffer);
	}
}

void RenderEngineIndirectDraw::ConstructPipelines() {
	Terra& terra = Terra::Get();

	VkDevice device = terra.Device().GetLogicalDevice();

	ConstructGraphicsPipelineLayout(device);
	CreateGraphicsPipelines(device, m_graphicsPipeline0, m_graphicsPipelines);

	DescriptorSetManager& descManager = terra.ComputeDesc();

	/*m_computePipeline.CreateComputePipelineLayout(
		device, descManager.GetDescriptorSetCount(), descManager.GetDescriptorSetLayouts()
	);
	m_computePipeline.CreateComputePipeline(device, m_shaderPath);*/
}

void RenderEngineIndirectDraw::UpdateModelBuffers(VkDeviceSize frameIndex) const noexcept {
	Terra::Get().Buffers().Update<false>(frameIndex);
}

void RenderEngineIndirectDraw::RecordModelDataSet(
	const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
) noexcept {
	auto graphicsPipeline = std::make_unique<GraphicsPipelineIndirectDraw>();
	// old currentModelCount hold the modelCountOffset value
	/*graphicsPipeline->ConfigureGraphicsPipeline(
		fragmentShader, static_cast<std::uint32_t>(std::size(models)),
		m_computePipeline.GetCurrentModelCount(), m_computePipeline.GetCounterCount()
	);*/

	//m_computePipeline.RecordIndirectArguments(models);

	if (!m_graphicsPipeline0)
		m_graphicsPipeline0 = std::move(graphicsPipeline);
	else
		m_graphicsPipelines.emplace_back(std::move(graphicsPipeline));
}

RenderEngineBase::WaitSemaphoreData RenderEngineIndirectDraw::GetWaitSemaphores(
) const noexcept {
	static VkSemaphore waitSemaphores[2]{};
	waitSemaphores[0] = Terra::Get().Compute().SyncObj().GetFrontSemaphore();
	waitSemaphores[1] = Terra::Get().Graphics().SyncObj().GetFrontSemaphore();

	static VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	return { waitSemaphores, waitStages };
}

void RenderEngineIndirectDraw::CreateBuffers(VkDevice device) noexcept {
	//m_computePipeline.CreateBuffers(device);
}

void RenderEngineIndirectDraw::_bindResourcesToMemory(VkDevice device) {
	//m_computePipeline.BindResourceToMemory(device);
}

void RenderEngineIndirectDraw::CopyData() noexcept {
	//m_computePipeline.CopyData();
}

void RenderEngineIndirectDraw::_recordCopy(VkCommandBuffer transferBuffer) noexcept {
	//m_computePipeline.RecordCopy(transferBuffer);
}

void RenderEngineIndirectDraw::_releaseUploadResources() noexcept {
	//m_computePipeline.ReleaseUploadResources();
}

void RenderEngineIndirectDraw::AcquireOwnerShipCompute(
	VkCommandBuffer computeCmdBuffer
) noexcept {
	//m_computePipeline.AcquireOwnerShip(computeCmdBuffer);
}

void RenderEngineIndirectDraw::_releaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept {
	//m_computePipeline.ReleaseOwnership(transferCmdBuffer);
}

void RenderEngineIndirectDraw::ExecuteComputeStage(size_t frameIndex) {
	Terra::Queue& compute = Terra::Get().Compute();
	VKCommandBuffer& computeCmdBuffer = compute.CmdBuffer();

	//computeCmdBuffer.ResetBuffer(frameIndex);

	//const VkCommandBuffer vkComputeCommandBuffer = computeCmdBuffer.GetCommandBuffer(
	//	frameIndex
	//);

	/*m_computePipeline.ResetCounterBuffer(vkComputeCommandBuffer, frameIndex);
	m_computePipeline.BindComputePipeline(vkComputeCommandBuffer, frameIndex);
	m_computePipeline.DispatchCompute(vkComputeCommandBuffer);

	computeCmdBuffer.CloseBuffer(frameIndex);
	compute.Que().SubmitCommandBuffer(vkComputeCommandBuffer, compute.SyncObj().GetFrontSemaphore());*/
}

// Individual Draw
RenderEngineIndividualDraw::RenderEngineIndividualDraw(VkDevice device, QueueIndicesTG queueIndices)
	: RenderEngineVertexShader{ device, queueIndices } {}

void RenderEngineIndividualDraw::RecordDrawCommands(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	// One Pipeline needs to be bound before Descriptors can be bound.
	//m_graphicsPipeline0->Bind(graphicsCmdBuffer);
	BindGraphicsBuffers(graphicsCmdBuffer, frameIndex);

	//m_graphicsPipeline0->DrawModels(graphicsCmdBuffer, m_modelArguments);

	for (auto& graphicsPipeline : m_graphicsPipelines) {
		//graphicsPipeline->Bind(graphicsCmdBuffer);
		//graphicsPipeline->DrawModels(graphicsCmdBuffer, m_modelArguments);
	}
}

void RenderEngineIndividualDraw::ConstructPipelines() {
	VkDevice device = Terra::Get().Device().GetLogicalDevice();

	ConstructGraphicsPipelineLayout(device);
	CreateGraphicsPipelines(device, m_graphicsPipeline0, m_graphicsPipelines);
}

void RenderEngineIndividualDraw::UpdateModelBuffers(VkDeviceSize frameIndex) const noexcept {
	Terra::Get().Buffers().Update<true>(frameIndex);
}

void RenderEngineIndividualDraw::RecordModelDataSet(
	const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
) noexcept {
	auto graphicsPipeline = std::make_unique<GraphicsPipelineIndividualDraw>();
	/*graphicsPipeline->ConfigureGraphicsPipeline(
		fragmentShader, static_cast<std::uint32_t>(std::size(models)),
		std::size(m_modelArguments)
	);*/

	RecordModelArguments(models);

	if (!m_graphicsPipeline0)
		m_graphicsPipeline0 = std::move(graphicsPipeline);
	else
		m_graphicsPipelines.emplace_back(std::move(graphicsPipeline));
}

void RenderEngineIndividualDraw::RecordModelArguments(
	const std::vector<std::shared_ptr<IModel>>& models
) noexcept {
	for (const auto& model : models) {
		VkDrawIndexedIndirectCommand arguments{
			.indexCount = model->GetIndexCount(),
			.instanceCount = 1u,
			.firstIndex = model->GetIndexOffset(),
			.vertexOffset = 0u,
			.firstInstance = static_cast<std::uint32_t>(std::size(m_modelArguments))
		};

		m_modelArguments.emplace_back(arguments);
	}
}
