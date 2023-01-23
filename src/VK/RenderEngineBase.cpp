#include <RenderEngineBase.hpp>

#include <Terra.hpp>

void RenderEngineBase::Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) {
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

void RenderEngineBase::ExecutePostRenderStage() {
	Terra::graphicsSyncObjects->AdvanceSyncObjectsInQueue();
	Terra::computeSyncObjects->AdvanceSemaphoreInQueue();

	Terra::graphicsSyncObjects->WaitForFrontFence();
	Terra::graphicsSyncObjects->ResetFrontFence();
}

std::unique_ptr<PipelineLayout> RenderEngineBase::CreateGraphicsPipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) const noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}

void RenderEngineBase::ExecutePreGraphicsStage(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
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

void RenderEngineBase::ConstructGraphicsPipelineLayout(
	VkDevice device, std::uint32_t frameCount
) {
	m_graphicsPipelineLayout = CreateGraphicsPipelineLayout(
		device, frameCount, Terra::graphicsDescriptorSet->GetDescriptorSetLayouts()
	);
}

void RenderEngineBase::BindGraphicsDescriptorSets(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	VkDescriptorSet descSets[] = { Terra::graphicsDescriptorSet->GetDescriptorSet(frameIndex) };
	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphicsPipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);
}
