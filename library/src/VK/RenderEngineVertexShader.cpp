#include <RenderEngineVertexShader.hpp>

// VS Individual
RenderEngineVSIndividual::RenderEngineVSIndividual(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineVS{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayout(m_graphicsDescriptorBuffers);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();
}

ModelManagerVSIndividual RenderEngineVSIndividual::GetModelManager(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
	[[maybe_unused]] StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) {
	return ModelManagerVSIndividual{ deviceManager.GetLogicalDevice(), memoryManager, frameCount };
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

std::uint32_t RenderEngineVSIndividual::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	// Add a mesh Bundle will update the Vertex and Index buffers. So, must wait for the queue to
	// finish.
	m_graphicsQueue.WaitForQueueToFinish();

	return m_modelManager.AddMeshBundle(std::move(meshBundle), m_stagingManager);
}

void RenderEngineVSIndividual::Render(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea
) {
	// Wait for the previous Graphics command buffer to finish.
	m_graphicsQueue.WaitForSubmission(frameIndex);

	Update(static_cast<VkDeviceSize>(frameIndex));

	// Transfer Phase
	const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

		m_stagingManager.Copy(transferCmdBufferScope);

		m_stagingManager.ReleaseOwnership(transferCmdBufferScope, m_transferQueue.GetFamilyIndex());
	}

	const VKSemaphore& transferWaitSemaphore = m_transferWait.at(frameIndex);

	{
		QueueSubmitBuilder<0u, 1u> transferSubmitBuilder{};
		transferSubmitBuilder.SignalSemaphore(transferWaitSemaphore).CommandBuffer(transferCmdBuffer);

		m_transferQueue.SubmitCommandBuffer(transferSubmitBuilder);
	}

	// Compute Phase (Not using atm)

	// Graphics Phase
	const VKCommandBuffer& graphicsCmdBuffer = m_graphicsQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope graphicsCmdBufferScope{ graphicsCmdBuffer };

		m_stagingManager.AcquireOwnership(
			graphicsCmdBufferScope, m_graphicsQueue.GetFamilyIndex(), m_transferQueue.GetFamilyIndex()
		);

		m_viewportAndScissors.BindViewportAndScissor(graphicsCmdBufferScope);

		VkDescriptorBuffer::BindDescriptorBuffer(
			m_graphicsDescriptorBuffers.at(frameIndex), graphicsCmdBufferScope,
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelManager.GetGraphicsPipelineLayout()
		);

		BeginRenderPass(graphicsCmdBufferScope, frameBuffer, renderArea);

		m_modelManager.Draw(graphicsCmdBufferScope);
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

// VS Indirect
RenderEngineVSIndirect::RenderEngineVSIndirect(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineVS{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayoutVS(m_graphicsDescriptorBuffers);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();
}

ModelManagerVSIndirect RenderEngineVSIndirect::GetModelManager(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
	StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
) {
	return ModelManagerVSIndirect{
		deviceManager.GetLogicalDevice(), memoryManager, stagingBufferMan,
		deviceManager.GetQueueFamilyManager().GetAllIndices(), frameCount
	};
}

std::uint32_t RenderEngineVSIndirect::AddModel(
	std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader
) {
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModel(std::move(model), fragmentShader);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBufferVS(m_graphicsDescriptorBuffers);

	// CS stuffs here.

	return index;
}

std::uint32_t RenderEngineVSIndirect::AddModelBundle(
	std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
) {
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(std::move(modelBundle), fragmentShader);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBufferVS(m_graphicsDescriptorBuffers);

	// CS stuffs here.

	return index;
}

std::uint32_t RenderEngineVSIndirect::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	// Add a mesh Bundle will update the Vertex and Index buffers. So, must wait for the queue to
	// finish.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddMeshBundle(std::move(meshBundle), m_stagingManager);

	//m_modelManager.SetDescriptorBufferCSOfModels()

	return index;
}

void RenderEngineVSIndirect::Render(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea
) {

}
