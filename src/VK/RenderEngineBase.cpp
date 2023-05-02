#include <RenderEngineBase.hpp>

#include <Terra.hpp>

RenderEngineBase::RenderEngineBase(VkDevice device) noexcept
	: m_renderPass{ device }, m_depthBuffer{ device } {
	m_depthBuffer.AllocateForMaxResolution(device, 7680u, 4320u);
}

void RenderEngineBase::Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) {
	vkCmdEndRenderPass(graphicsCmdBuffer);

	Terra::graphicsCmdBuffer->CloseBuffer(frameIndex);

	const auto& waitSemaphores = GetWaitSemaphores();
	static auto semaphoreCount = static_cast<std::uint32_t>(std::size(waitSemaphores.first));

	Terra::graphicsQueue->SubmitCommandBuffer(
		graphicsCmdBuffer, Terra::graphicsSyncObjects->GetFrontFence(),
		semaphoreCount, std::data(waitSemaphores.first), std::data(waitSemaphores.second)
	);
	Terra::swapChain->PresentImage(static_cast<std::uint32_t>(frameIndex));
}

void RenderEngineBase::ExecutePostRenderStage() {
	Terra::graphicsSyncObjects->AdvanceSyncObjectsInQueue();
	Terra::computeSyncObjects->AdvanceSemaphoreInQueue();

	Terra::graphicsSyncObjects->WaitForFrontFence();
	Terra::graphicsSyncObjects->ResetFrontFence();
}

void RenderEngineBase::ExecutePreGraphicsStage(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	Terra::graphicsCmdBuffer->ResetBuffer(frameIndex);

	vkCmdSetViewport(graphicsCmdBuffer, 0u, 1u, m_viewportAndScissor.GetViewportRef());
	vkCmdSetScissor(graphicsCmdBuffer, 0u, 1u, m_viewportAndScissor.GetScissorRef());

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = m_backgroundColour;
	clearValues[1].depthStencil = { 1.f, 0 };

	VkRenderPassBeginInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = GetRenderPass(),
		.framebuffer = Terra::swapChain->GetFramebuffer(frameIndex),
		.renderArea = { VkOffset2D{ 0, 0 }, Terra::swapChain->GetSwapExtent() },
		.clearValueCount = static_cast<std::uint32_t>(std::size(clearValues)),
		.pClearValues = std::data(clearValues)
	};

	vkCmdBeginRenderPass(graphicsCmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderEngineBase::ConstructGraphicsPipelineLayout(VkDevice device) {
	DescriptorSetManager const* descManager = Terra::graphicsDescriptorSet.get();

	m_graphicsPipelineLayout = CreateGraphicsPipelineLayout(
		device, descManager->GetDescriptorSetCount(), descManager->GetDescriptorSetLayouts()
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

void RenderEngineBase::CreateRenderPass(VkDevice device, VkFormat swapchainFormat) {
	m_renderPass.CreateRenderPass(device, swapchainFormat, m_depthBuffer.GetDepthFormat());
}

VkImageView RenderEngineBase::GetDepthImageView() const noexcept {
	return m_depthBuffer.GetDepthImageView();
}

VkRenderPass RenderEngineBase::GetRenderPass() const noexcept {
	return m_renderPass.GetRenderPass();
}

void RenderEngineBase::ResizeViewportAndScissor(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_viewportAndScissor.Resize(width, height);
}

void RenderEngineBase::CreateDepthBuffer(
	VkDevice device, std::uint32_t width, std::uint32_t height
) {
	m_depthBuffer.CreateDepthBuffer(device, width, height);
}

void RenderEngineBase::CleanUpDepthBuffer() noexcept {
	m_depthBuffer.CleanUp();
}

void RenderEngineBase::ExecutePreRenderStage(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	ExecutePreGraphicsStage(graphicsCmdBuffer, frameIndex);
}

RenderEngineBase::WaitSemaphoreData RenderEngineBase::GetWaitSemaphores(
) const noexcept {
	static VkSemaphore waitSemaphores[1]{};
	waitSemaphores[0] = Terra::graphicsSyncObjects->GetFrontSemaphore();

	static VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	return { waitSemaphores, waitStages };
}

void RenderEngineBase::AddGVerticesAndIndices(
	[[maybe_unused]] VkDevice device, [[maybe_unused]] std::vector<Vertex>&& gVertices,
	[[maybe_unused]] std::vector<std::uint32_t>&& gIndices
) noexcept {}

void RenderEngineBase::RecordModelDataSet(
	[[maybe_unused]] const std::vector<std::shared_ptr<IModel>>& models,
	[[maybe_unused]] const std::wstring& fragmentShader
) noexcept {}

void RenderEngineBase::AddMeshletModelSet(
	[[maybe_unused]] std::vector<MeshletModel>& meshletModels,
	[[maybe_unused]] const std::wstring& pixelShader
) noexcept {}

void RenderEngineBase::AddGVerticesAndPrimIndices(
	[[maybe_unused]] VkDevice device, [[maybe_unused]] std::vector<Vertex>&& gVertices,
	[[maybe_unused]] std::vector<std::uint32_t>&& gVerticesIndices,
	[[maybe_unused]] std::vector<std::uint32_t>&& gPrimIndices
) noexcept {}
