#include <RenderEngineMeshShader.hpp>

void RenderEngineMSDeviceExtension::SetDeviceExtensions(
	VkDeviceExtensionManager& extensionManager
) noexcept {
	RenderEngineDeviceExtension::SetDeviceExtensions(extensionManager);

	extensionManager.AddExtensions(ModelBundleMSIndividual::GetRequiredExtensions());
}

RenderEngineMS::RenderEngineMS(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayout(
		m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex, s_fragmentShaderSetLayoutIndex
	);
	SetCommonGraphicsDescriptorBufferLayout(VK_SHADER_STAGE_MESH_BIT_EXT);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	if (!std::empty(m_graphicsDescriptorBuffers))
		m_modelManager.CreatePipelineLayout(m_graphicsDescriptorBuffers.front());

	m_cameraManager.CreateBuffer({}, static_cast<std::uint32_t>(frameCount));
	m_cameraManager.SetDescriptorBufferGraphics(
		m_graphicsDescriptorBuffers, s_cameraBindingSlot, s_vertexShaderSetLayoutIndex
	);
}

std::uint32_t RenderEngineMS::AddModelBundle(
	std::shared_ptr<ModelBundleMS>&& modelBundle, const ShaderName& fragmentShader
) {
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), fragmentShader, m_temporaryDataBuffer
	);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBufferOfModels(
		m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex, s_fragmentShaderSetLayoutIndex
	);

	return index;
}

std::uint32_t RenderEngineMS::AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle)
{
	// Add a mesh Bundle will update the Vertex, VertexIndices and PrimIndices buffers.
	// So, must wait for the queue to finish.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_modelManager.SetDescriptorBufferOfMeshes(m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex);

	return index;
}

void RenderEngineMS::Render(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
	std::uint64_t frameNumber, const VKSemaphore& imageWaitSemaphore
) {
	// Wait for the previous Graphics command buffer to finish.
	m_graphicsQueue.WaitForSubmission(frameIndex);
	// It should be okay to clear the data now that the frame has finished
	// its submission.
	m_temporaryDataBuffer.Clear(frameIndex);

	// This should be fine. But putting this as a reminder, that
	// the presentation engine might still be running and using some resources.
	Update(static_cast<VkDeviceSize>(frameIndex));

	// Transfer Phase
	const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

		m_stagingManager.CopyAndClear(transferCmdBufferScope);
		m_modelManager.CopyTempBuffers(transferCmdBuffer);

		m_stagingManager.ReleaseOwnership(transferCmdBufferScope, m_transferQueue.GetFamilyIndex());
	}

	const VKSemaphore& transferWaitSemaphore = m_transferWait[frameIndex];

	{
		QueueSubmitBuilder<1u, 1u> transferSubmitBuilder{};
		transferSubmitBuilder
			.SignalSemaphore(transferWaitSemaphore, frameNumber)
			// The present queue could still be using some resources. So, need to wait.
			.WaitSemaphore(imageWaitSemaphore, VK_PIPELINE_STAGE_TRANSFER_BIT)
			.CommandBuffer(transferCmdBuffer);

		m_transferQueue.SubmitCommandBuffer(transferSubmitBuilder);

		m_temporaryDataBuffer.SetUsed(frameIndex);
	}

	// Compute Phase (Not using atm)

	// Graphics Phase
	const VKCommandBuffer& graphicsCmdBuffer = m_graphicsQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope graphicsCmdBufferScope{ graphicsCmdBuffer };

		m_stagingManager.AcquireOwnership(
			graphicsCmdBufferScope, m_graphicsQueue.GetFamilyIndex(), m_transferQueue.GetFamilyIndex()
		);

		m_textureStorage.TransitionQueuedTextures(graphicsCmdBufferScope);

		m_viewportAndScissors.BindViewportAndScissor(graphicsCmdBufferScope);

		VkDescriptorBuffer::BindDescriptorBuffer(
			m_graphicsDescriptorBuffers[frameIndex], graphicsCmdBufferScope,
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelManager.GetGraphicsPipelineLayout()
		);

		BeginRenderPass(graphicsCmdBufferScope, frameBuffer, renderArea);

		m_modelManager.Draw(graphicsCmdBufferScope);

		m_renderPass.EndPass(graphicsCmdBuffer.Get());
	}

	const VKSemaphore& graphicsWaitSemaphore = m_graphicsWait[frameIndex];

	{
		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalSemaphore(graphicsWaitSemaphore)
			// The graphics queue should wait for the transfer queue to finish and then start the
			// Input Assembler Stage.
			.WaitSemaphore(transferWaitSemaphore, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT, frameNumber)
			.CommandBuffer(graphicsCmdBuffer);

		VKFence& signalFence = m_graphicsQueue.GetFence(frameIndex);
		signalFence.Reset();

		m_graphicsQueue.SubmitCommandBuffer(graphicsSubmitBuilder, signalFence);
	}
}

ModelManagerMS RenderEngineMS::GetModelManager(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
	StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
) {
	return ModelManagerMS{
		deviceManager.GetLogicalDevice(), memoryManager, stagingBufferMan, frameCount
	};
}
