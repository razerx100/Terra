#include <Shader.hpp>

#include <Terra.hpp>
#include <RenderEngineIndirectDraw.hpp>

void RenderEngineIndirectDraw::ExecutePreRenderStage(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	// Compute Stage
	Terra::computeCmdBuffer->ResetBuffer(frameIndex);

	const VkCommandBuffer computeCommandBuffer = Terra::computeCmdBuffer->GetCommandBuffer(
		frameIndex
	);

	m_renderPipeline->ResetCounterBuffer(computeCommandBuffer, frameIndex);
	// Compute Pipeline doesn't need to be changed for different Graphics Pipelines
	vkCmdBindPipeline(
		computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePSO->GetPipeline()
	);

	VkDescriptorSet descSets[] = { Terra::computeDescriptorSet->GetDescriptorSet(frameIndex) };
	vkCmdBindDescriptorSets(
		computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		m_computePipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);

	m_renderPipeline->DispatchCompute(computeCommandBuffer, frameIndex);

	Terra::computeCmdBuffer->CloseBuffer(frameIndex);
	Terra::computeQueue->SubmitCommandBuffer(
		computeCommandBuffer, Terra::computeSyncObjects->GetFrontSemaphore()
	);

	// Garphics Stage
	Terra::graphicsCmdBuffer->ResetBuffer(frameIndex);

	vkCmdSetViewport(
		graphicsCmdBuffer, 0u, 1u, Terra::viewportAndScissor->GetViewportRef()
	);
	vkCmdSetScissor(
		graphicsCmdBuffer, 0u, 1u, Terra::viewportAndScissor->GetScissorRef()
	);

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = m_backgroundColour;
	clearValues[1].depthStencil = { 1.f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderPass = Terra::renderPass->GetRenderPass();
	renderPassInfo.framebuffer = Terra::swapChain->GetFramebuffer(frameIndex);
	renderPassInfo.renderArea.extent = Terra::swapChain->GetSwapExtent();
	renderPassInfo.clearValueCount = static_cast<std::uint32_t>(std::size(clearValues));
	renderPassInfo.pClearValues = std::data(clearValues);

	vkCmdBeginRenderPass(graphicsCmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderEngineIndirectDraw::RecordDrawCommands(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	// One Pipeline needs to be bound before Descriptors can be bound.
	m_renderPipeline->BindGraphicsPipeline(graphicsCmdBuffer);

	VkDescriptorSet descSets[] = { Terra::graphicsDescriptorSet->GetDescriptorSet(frameIndex) };
	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphicsPipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);

	Terra::bufferManager->BindVertexBuffer(graphicsCmdBuffer);
	m_renderPipeline->DrawModels(graphicsCmdBuffer, frameIndex);
}

void RenderEngineIndirectDraw::Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) {
	vkCmdEndRenderPass(graphicsCmdBuffer);

	Terra::graphicsCmdBuffer->CloseBuffer(frameIndex);

	VkSemaphore waitSemaphores[] = {
		Terra::computeSyncObjects->GetFrontSemaphore(),
		Terra::graphicsSyncObjects->GetFrontSemaphore()
	};
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	Terra::graphicsQueue->SubmitCommandBuffer(
		graphicsCmdBuffer, Terra::graphicsSyncObjects->GetFrontFence(),
		2u, waitSemaphores, waitStages
	);
	Terra::swapChain->PresentImage(static_cast<std::uint32_t>(frameIndex));
}

void RenderEngineIndirectDraw::ExecutePostRenderStage() {
	Terra::graphicsSyncObjects->AdvanceSyncObjectsInQueue();
	Terra::computeSyncObjects->AdvanceSemaphoreInQueue();

	Terra::graphicsSyncObjects->WaitForFrontFence();
	Terra::graphicsSyncObjects->ResetFrontFence();
}

void RenderEngineIndirectDraw::ConstructPipelines(std::uint32_t frameCount) {
	VkDevice device = Terra::device->GetLogicalDevice();

	m_graphicsPipelineLayout = CreateGraphicsPipelineLayout(
		device, frameCount, Terra::graphicsDescriptorSet->GetDescriptorSetLayouts()
	);
	m_renderPipeline->ConfigureGraphicsPipelineObject(
		device, m_graphicsPipelineLayout->GetLayout(), Terra::renderPass->GetRenderPass(),
		m_shaderPath, L"FragmentShader.spv"
	);

	m_computePipelineLayout = CreateComputePipelineLayout(
		device, frameCount, Terra::computeDescriptorSet->GetDescriptorSetLayouts()
	);
	m_computePSO = CreateComputePipeline(device, m_computePipelineLayout->GetLayout());
}

void RenderEngineIndirectDraw::InitiatePipelines(
	VkDevice device, std::uint32_t bufferCount,
	std::vector<std::uint32_t> computeAndGraphicsQueueIndices
) noexcept {
	m_renderPipeline = std::make_unique<RenderPipelineIndirectDraw>(
		device, bufferCount, computeAndGraphicsQueueIndices
		);
}

void RenderEngineIndirectDraw::RecordModelData(
	const std::vector<std::shared_ptr<IModel>>& models
) noexcept {
	m_renderPipeline->RecordIndirectArguments(models);
}

void RenderEngineIndirectDraw::CreateBuffers(VkDevice device) noexcept {
	m_renderPipeline->CreateBuffers(device);
}

void RenderEngineIndirectDraw::BindResourcesToMemory(VkDevice device) {
	m_renderPipeline->BindResourceToMemory(device);
}

void RenderEngineIndirectDraw::CopyData() noexcept {
	m_renderPipeline->CopyData();
}

void RenderEngineIndirectDraw::RecordCopy(VkCommandBuffer copyBuffer) noexcept {
	m_renderPipeline->RecordCopy(copyBuffer);
}

void RenderEngineIndirectDraw::ReleaseUploadResources() noexcept {
	m_renderPipeline->ReleaseUploadResources();
}

void RenderEngineIndirectDraw::AcquireOwnerShip(
	VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_renderPipeline->AcquireOwnerShip(cmdBuffer, srcQueueIndex, dstQueueIndex);
}

void RenderEngineIndirectDraw::ReleaseOwnership(
	VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_renderPipeline->ReleaseOwnership(copyCmdBuffer, srcQueueIndex, dstQueueIndex);
}

std::unique_ptr<PipelineLayout> RenderEngineIndirectDraw::CreateGraphicsPipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) const noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}

std::unique_ptr<PipelineLayout> RenderEngineIndirectDraw::CreateComputePipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) const noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages
	// Doesn't do anything different now but might, in the future idk

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}

std::unique_ptr<VkPipelineObject> RenderEngineIndirectDraw::CreateComputePipeline(
	VkDevice device, VkPipelineLayout computeLayout
) const noexcept {
	auto cs = std::make_unique<Shader>(device);
	cs->CreateShader(device, m_shaderPath + L"ComputeShader.spv");

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateComputePipeline(device, computeLayout, cs->GetByteCode());

	return pso;
}
