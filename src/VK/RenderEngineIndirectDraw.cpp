
#include <Terra.hpp>
#include <RenderEngineIndirectDraw.hpp>

RenderEngineIndirectDraw::RenderEngineIndirectDraw(Args& arguments)
	: m_computePipeline{
		arguments.device.value(), arguments.bufferCount.value(),
		std::move(arguments.computeAndGraphicsQueueIndices.value())
	} {}

void RenderEngineIndirectDraw::ExecutePreRenderStage(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	// Compute Stage
	Terra::computeCmdBuffer->ResetBuffer(frameIndex);

	const VkCommandBuffer computeCommandBuffer = Terra::computeCmdBuffer->GetCommandBuffer(
		frameIndex
	);

	m_computePipeline.ResetCounterBuffer(computeCommandBuffer, frameIndex);
	m_computePipeline.BindComputePipeline(computeCommandBuffer, frameIndex);
	m_computePipeline.DispatchCompute(computeCommandBuffer);

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

	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = Terra::renderPass->GetRenderPass(),
		.framebuffer = Terra::swapChain->GetFramebuffer(frameIndex),
		.renderArea = { VkOffset2D{ 0, 0 }, Terra::swapChain->GetSwapExtent() },
		.clearValueCount = static_cast<std::uint32_t>(std::size(clearValues)),
		.pClearValues = std::data(clearValues)
	};

	vkCmdBeginRenderPass(graphicsCmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderEngineIndirectDraw::RecordDrawCommands(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	// One Pipeline needs to be bound before Descriptors can be bound.
	m_graphicsPipeline0->BindGraphicsPipeline(graphicsCmdBuffer);

	VkDescriptorSet descSets[] = { Terra::graphicsDescriptorSet->GetDescriptorSet(frameIndex) };
	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphicsPipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);

	Terra::vertexManager->BindVertices(graphicsCmdBuffer);

	const VkBuffer argumentBuffer = m_computePipeline.GetArgumentBuffer(frameIndex);
	const VkBuffer counterBuffer = m_computePipeline.GetCounterBuffer(frameIndex);

	m_graphicsPipeline0->DrawModels(graphicsCmdBuffer, argumentBuffer, counterBuffer);

	for (auto& graphicsPipeline : m_graphicsPipelines) {
		graphicsPipeline->BindGraphicsPipeline(graphicsCmdBuffer);
		graphicsPipeline->DrawModels(graphicsCmdBuffer, argumentBuffer, counterBuffer);
	}
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

	m_graphicsPipeline0->CreateGraphicsPipeline(
		device, m_graphicsPipelineLayout->GetLayout(), Terra::renderPass->GetRenderPass(),
		m_shaderPath
	);
	for (auto& graphicsPipeline : m_graphicsPipelines)
		graphicsPipeline->CreateGraphicsPipeline(
			device, m_graphicsPipelineLayout->GetLayout(), Terra::renderPass->GetRenderPass(),
			m_shaderPath
		);

	m_computePipeline.CreateComputePipelineLayout(
		device, frameCount, Terra::computeDescriptorSet->GetDescriptorSetLayouts()
	);
	m_computePipeline.CreateComputePipeline(device, m_shaderPath);
}

void RenderEngineIndirectDraw::RecordModelDataSet(
	const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
) noexcept {
	auto graphicsPipeline = std::make_unique<GraphicsPipelineIndirectDraw>();
	graphicsPipeline->ConfigureGraphicsPipeline(
		fragmentShader, static_cast<std::uint32_t>(std::size(models)),
		m_computePipeline.GetCurrentModelCount()
	);

	// old currentModelCount hold the modelCountOffset value
	m_computePipeline.RecordIndirectArguments(models);

	if (!m_graphicsPipeline0)
		m_graphicsPipeline0 = std::move(graphicsPipeline);
	else
		m_graphicsPipelines.emplace_back(std::move(graphicsPipeline));
}

void RenderEngineIndirectDraw::CreateBuffers(VkDevice device) noexcept {
	m_computePipeline.CreateBuffers(device);
}

void RenderEngineIndirectDraw::BindResourcesToMemory(VkDevice device) {
	m_computePipeline.BindResourceToMemory(device);
}

void RenderEngineIndirectDraw::CopyData() noexcept {
	m_computePipeline.CopyData();
}

void RenderEngineIndirectDraw::RecordCopy(VkCommandBuffer copyBuffer) noexcept {
	m_computePipeline.RecordCopy(copyBuffer);
}

void RenderEngineIndirectDraw::ReleaseUploadResources() noexcept {
	m_computePipeline.ReleaseUploadResources();
}

void RenderEngineIndirectDraw::AcquireOwnerShip(
	VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_computePipeline.AcquireOwnerShip(cmdBuffer, srcQueueIndex, dstQueueIndex);
}

void RenderEngineIndirectDraw::ReleaseOwnership(
	VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_computePipeline.ReleaseOwnership(copyCmdBuffer, srcQueueIndex, dstQueueIndex);
}

std::unique_ptr<PipelineLayout> RenderEngineIndirectDraw::CreateGraphicsPipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) const noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}
