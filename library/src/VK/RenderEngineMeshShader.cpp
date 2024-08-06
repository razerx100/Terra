#include <RenderEngineMeshShader.hpp>

void RenderEngineMSDeviceExtension::SetDeviceExtensions(
	VkDeviceExtensionManager& extensionManager
) noexcept {
	RenderEngineDeviceExtension::SetDeviceExtensions(extensionManager);

	extensionManager.AddExtensions(ModelBundleMS::GetRequiredExtensions());
}

RenderEngineMS::RenderEngineMS(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayout(m_graphicsDescriptorBuffers);
	SetCommonGraphicsDescriptorBufferLayout(VK_SHADER_STAGE_MESH_BIT_EXT);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	if (!std::empty(m_graphicsDescriptorBuffers))
		m_modelManager.CreatePipelineLayout(m_graphicsDescriptorBuffers.front());

	m_cameraManager.CreateBuffer({}, static_cast<std::uint32_t>(frameCount));
	m_cameraManager.SetDescriptorBufferGraphics(m_graphicsDescriptorBuffers, s_cameraBindingSlot);
}

std::uint32_t RenderEngineMS::AddModel(std::shared_ptr<ModelMS>&& model, const ShaderName& fragmentShader)
{
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModel(
		std::move(model), fragmentShader, m_temporaryDataBuffer
	);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBufferOfModels(m_graphicsDescriptorBuffers);

	return index;
}

std::uint32_t RenderEngineMS::AddModelBundle(
	std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const ShaderName& fragmentShader
) {
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), fragmentShader, m_temporaryDataBuffer
	);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBufferOfModels(m_graphicsDescriptorBuffers);

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

	m_modelManager.SetDescriptorBufferOfMeshes(m_graphicsDescriptorBuffers);

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

	Update(static_cast<VkDeviceSize>(frameIndex));

	// Transfer Phase
	const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

		m_stagingManager.CopyAndClear(transferCmdBufferScope);
		m_modelManager.CopyTempBuffers(transferCmdBuffer);

		m_stagingManager.ReleaseOwnership(transferCmdBufferScope, m_transferQueue.GetFamilyIndex());
	}

	const VKSemaphore& transferWaitSemaphore = m_transferWait.at(frameIndex);

	{
		QueueSubmitBuilder<0u, 1u> transferSubmitBuilder{};
		transferSubmitBuilder
			.SignalSemaphore(transferWaitSemaphore, frameNumber)
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
			m_graphicsDescriptorBuffers.at(frameIndex), graphicsCmdBufferScope,
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_modelManager.GetGraphicsPipelineLayout()
		);

		BeginRenderPass(graphicsCmdBufferScope, frameBuffer, renderArea);

		m_modelManager.Draw(graphicsCmdBufferScope);

		m_renderPass.EndPass(graphicsCmdBuffer.Get());
	}

	const VKSemaphore& graphicsWaitSemaphore = m_graphicsWait.at(frameIndex);

	{
		QueueSubmitBuilder<2u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalSemaphore(graphicsWaitSemaphore)
			// The graphics queue should wait for the transfer queue to finish and then start the
			// Input Assembler Stage.
			.WaitSemaphore(transferWaitSemaphore, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT, frameNumber)
			// The image wait semaphore will most likely be a graphicsWaitSemaphore. But it should
			// be fine to wait on it on the COLOUR_ATTACHMENT_OUTPUT_BIT as that's when we will draw
			// on the frameBuffer. And we don't really need the frameBuffer before that. And the
			// graphics queue will signal once it is finished.
			// And since the swapchain doesn't support timeline semaphores, it won't be a
			// timeline semaphore.
			.WaitSemaphore(imageWaitSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
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
