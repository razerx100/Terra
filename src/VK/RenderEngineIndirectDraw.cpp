#include <PipelineConstructor.hpp>

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

	Terra::renderPipeline->ResetCounterBuffer(computeCommandBuffer, frameIndex);
	Terra::renderPipeline->BindComputePipeline(
		computeCommandBuffer, Terra::computeDescriptorSet->GetDescriptorSet(frameIndex)
	);
	Terra::renderPipeline->DispatchCompute(computeCommandBuffer, frameIndex);

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
	Terra::renderPipeline->BindGraphicsPipeline(
		graphicsCmdBuffer, Terra::graphicsDescriptorSet->GetDescriptorSet(frameIndex)
	);
	Terra::bufferManager->BindVertexBuffer(graphicsCmdBuffer);
	Terra::renderPipeline->DrawModels(graphicsCmdBuffer, frameIndex);
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

	auto graphicsLayout = CreateGraphicsPipelineLayout(
		device, frameCount, Terra::graphicsDescriptorSet->GetDescriptorSetLayouts()
	);
	auto graphicsPipeline = CreateGraphicsPipeline(
		device, graphicsLayout->GetLayout(), Terra::renderPass->GetRenderPass(),
		m_shaderPath
	);

	Terra::renderPipeline->AddGraphicsPipelineLayout(std::move(graphicsLayout));
	Terra::renderPipeline->AddGraphicsPipelineObject(std::move(graphicsPipeline));

	auto computeLayout = CreateComputePipelineLayout(
		device, frameCount, Terra::computeDescriptorSet->GetDescriptorSetLayouts()
	);
	auto computePipeline = CreateComputePipeline(
		device, computeLayout->GetLayout(), m_shaderPath
	);

	Terra::renderPipeline->AddComputePipelineLayout(std::move(computeLayout));
	Terra::renderPipeline->AddComputePipelineObject(std::move(computePipeline));
}
