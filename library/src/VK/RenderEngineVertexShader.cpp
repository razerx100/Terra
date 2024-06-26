#include <RenderEngineVertexShader.hpp>

RenderEngineVSIndividual::RenderEngineVSIndividual(
	VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
	VkQueueFamilyMananger const* queueFamilyManager, std::shared_ptr<ThreadPool> threadPool,
	size_t frameCount
) : RenderEngine{ physicalDevice, logicalDevice, queueFamilyManager, std::move(threadPool), frameCount },
	m_modelManager{
		logicalDevice, &m_memoryManager, static_cast<std::uint32_t>(frameCount)
	}
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayout(m_graphicsDescriptorBuffers);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();
}

std::uint32_t RenderEngineVSIndividual::AddModel(
	std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader
) {
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModel(std::move(model), fragmentShader);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBuffer(m_graphicsDescriptorBuffers);

	return index;
}

std::uint32_t RenderEngineVSIndividual::AddModelBundle(
	std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
) {
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(std::move(modelBundle), fragmentShader);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBuffer(m_graphicsDescriptorBuffers);

	return index;
}

void RenderEngineVSIndividual::Update(VkDeviceSize frameIndex)
{
	RenderEngine::Update(frameIndex);

	m_modelManager.UpdatePerFrame(frameIndex);
}

void RenderEngineVSIndividual::Render(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea
) {
	// Wait for the previous Graphics command buffer to finish.
	m_graphicsQueue.WaitForSubmission(frameIndex);

	Update(static_cast<VkDeviceSize>(frameIndex));

	// Transfer Phase
	VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

	{
		CommandBufferScope cmdBufferScope{ transferCmdBuffer };

		m_stagingManager.Copy(transferCmdBuffer);

		m_stagingManager.ReleaseOwnership(transferCmdBuffer, m_transferQueue.GetFamilyIndex());
	}

	const VKSemaphore& transferWaitSemaphore = m_transferWait.at(frameIndex);

	{
		QueueSubmitBuilder<0u, 1u> transferSubmitBuilder{};
		transferSubmitBuilder.SignalSemaphore(transferWaitSemaphore).CommandBuffer(transferCmdBuffer);

		m_transferQueue.SubmitCommandBuffer(transferSubmitBuilder);
	}

	// Compute Phase (Not using atm)

	// Graphics Phase
	VKCommandBuffer& graphicsCmdBuffer = m_graphicsQueue.GetCommandBuffer(frameIndex);

	{
		CommandBufferScope cmdBufferScope{ graphicsCmdBuffer };

		m_stagingManager.AcquireOwnership(
			graphicsCmdBuffer, m_graphicsQueue.GetFamilyIndex(), m_transferQueue.GetFamilyIndex()
		);

		m_viewportAndScissors.BindViewportAndScissor(graphicsCmdBuffer);

		VkDescriptorBuffer::BindDescriptorBuffer(
			m_graphicsDescriptorBuffers.at(frameIndex), graphicsCmdBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelManager.GetGraphicsPipelineLayout()
		);

		BeginRenderPass(graphicsCmdBuffer, frameBuffer, renderArea);

		m_modelManager.Draw(graphicsCmdBuffer);
	}

	const VKSemaphore& graphicsWaitSemaphore = m_graphicsWait.at(frameIndex);

	{
		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalSemaphore(graphicsWaitSemaphore)
			// The graphics queue should wait for the transfer queue finish and then start the
			// Input Assembler Stage.
			.WaitSemaphore(transferWaitSemaphore, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)
			.CommandBuffer(graphicsCmdBuffer);

		VKFence& signalFence = m_graphicsQueue.GetFence(frameIndex);
		signalFence.Reset();

		m_graphicsQueue.SubmitCommandBuffer(graphicsSubmitBuilder, signalFence);
	}
}
