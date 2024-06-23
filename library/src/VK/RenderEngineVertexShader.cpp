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

void RenderEngineVSIndividual::Render(VkDeviceSize frameIndex)
{
	const auto frameIndexUz = static_cast<size_t>(frameIndex);

	// Wait for the previous Graphics command buffer to finish.
	m_graphicsQueue.WaitForSubmission(frameIndexUz);

	// Transfer Phase
	VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndexUz);

	{
		// Do all kinda gpu copying here.
	}

	const VKSemaphore& transferWaitSemaphore = m_transferWait.at(frameIndexUz);

	{
		QueueSubmitBuilder<0u, 1u> transferSubmitBuilder{};
		transferSubmitBuilder.SignalSemaphore(transferWaitSemaphore).CommandBuffer(transferCmdBuffer);

		m_transferQueue.SubmitCommandBuffer(transferSubmitBuilder);
	}

	// Compute Phase (Not using atm)

	// Graphics Phase
	VKCommandBuffer& graphicsCmdBuffer = m_graphicsQueue.GetCommandBuffer(frameIndexUz);

	{
		// Do all kinda gpu rendering here.
	}

	const VKSemaphore& graphicsWaitSemaphore = m_graphicsWait.at(frameIndexUz);

	{
		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalSemaphore(graphicsWaitSemaphore)
			// The graphics queue should wait for the transfer queue finish and then start the
			// Input Assembler Stage.
			.WaitSemaphore(transferWaitSemaphore, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)
			.CommandBuffer(graphicsCmdBuffer);

		VKFence& signalFence = m_graphicsQueue.GetFence(frameIndexUz);
		signalFence.Reset();

		m_graphicsQueue.SubmitCommandBuffer(graphicsSubmitBuilder, signalFence);
	}
}
