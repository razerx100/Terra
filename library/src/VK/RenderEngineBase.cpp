#include <RenderEngineBase.hpp>

#include <Terra.hpp>

void RenderEngineBase::Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) {
	vkCmdEndRenderPass(graphicsCmdBuffer);

	Terra::Queue& graphics = Terra::Get().Graphics();

	graphics.Que().GetCommandBuffer(frameIndex).Close();

	const auto& waitSemaphores = GetWaitSemaphores();
	static auto semaphoreCount = static_cast<std::uint32_t>(std::size(waitSemaphores.first));

	//graphics.Que().SubmitCommandBuffer(
	//	graphicsCmdBuffer, graphics.SyncObj().GetFrontFence(),
	//	semaphoreCount, std::data(waitSemaphores.first), std::data(waitSemaphores.second)
	//);
	Terra::Get().Swapchain().PresentImage(static_cast<std::uint32_t>(frameIndex));
}

void RenderEngineBase::ExecutePostRenderStage() {
	VkSyncObjects& graphicsSync = Terra::Get().Graphics().SyncObj();

	graphicsSync.AdvanceSyncObjectsInQueue();
	Terra::Get().Compute().SyncObj().AdvanceSemaphoreInQueue();

	graphicsSync.WaitForFrontFence();
	graphicsSync.ResetFrontFence();
}

void RenderEngineBase::ExecutePreGraphicsStage(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	Terra& terra = Terra::Get();
	terra.Graphics().Que().GetCommandBuffer(frameIndex).Reset();

	vkCmdSetViewport(graphicsCmdBuffer, 0u, 1u, m_viewportAndScissor.GetViewportRef());
	vkCmdSetScissor(graphicsCmdBuffer, 0u, 1u, m_viewportAndScissor.GetScissorRef());

	SwapchainManager& swapchain = terra.Swapchain();

	swapchain.BeginRenderPass(graphicsCmdBuffer, m_backgroundColour, frameIndex);
}

void RenderEngineBase::ConstructGraphicsPipelineLayout(VkDevice device) {
	DescriptorSetManager& descManager = Terra::Get().GraphicsDesc();

	m_graphicsPipelineLayout = CreateGraphicsPipelineLayout(
		device, descManager.GetDescriptorSetCount(), descManager.GetDescriptorSetLayouts()
	);
}

void RenderEngineBase::BindGraphicsDescriptorSets(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	VkDescriptorSet descSets[] = { Terra::Get().GraphicsDesc().GetDescriptorSet(frameIndex) };
	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphicsPipelineLayout->Get(), 0u, 1u,
		descSets, 0u, nullptr
	);
}

void RenderEngineBase::ResizeViewportAndScissor(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_viewportAndScissor.Resize(width, height);
}

void RenderEngineBase::ExecutePreRenderStage(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	ExecutePreGraphicsStage(graphicsCmdBuffer, frameIndex);
}

RenderEngineBase::WaitSemaphoreData RenderEngineBase::GetWaitSemaphores(
) const noexcept {
	static VkSemaphore waitSemaphores[1]{};
	waitSemaphores[0] = Terra::Get().Graphics().SyncObj().GetFrontSemaphore();

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
	[[maybe_unused]] const std::vector<std::shared_ptr<Model>>& models,
	[[maybe_unused]] const std::wstring& fragmentShader
) noexcept {}

void RenderEngineBase::AddMeshletModelSet(
	[[maybe_unused]] std::vector<MeshletModel>& meshletModels,
	[[maybe_unused]] const std::wstring& fragmentShader
) noexcept {}

void RenderEngineBase::AddGVerticesAndPrimIndices(
	[[maybe_unused]] VkDevice device, [[maybe_unused]] std::vector<Vertex>&& gVertices,
	[[maybe_unused]] std::vector<std::uint32_t>&& gVerticesIndices,
	[[maybe_unused]] std::vector<std::uint32_t>&& gPrimIndices
) noexcept {}
