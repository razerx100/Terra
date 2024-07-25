#include <RenderEngineVertexShader.hpp>

// VS Individual
RenderEngineVSIndividual::RenderEngineVSIndividual(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayout(m_graphicsDescriptorBuffers);
	SetCommonGraphicsDescriptorBufferLayout(VK_SHADER_STAGE_VERTEX_BIT);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	if (!std::empty(m_graphicsDescriptorBuffers))
		m_modelManager.CreatePipelineLayout(m_graphicsDescriptorBuffers.front());

	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.CreateBuffer({}, static_cast<std::uint32_t>(frameCount));
	m_cameraManager.SetDescriptorBufferGraphics(m_graphicsDescriptorBuffers, s_cameraBindingSlot);
}

ModelManagerVSIndividual RenderEngineVSIndividual::GetModelManager(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
	[[maybe_unused]] StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) {
	return ModelManagerVSIndividual{ deviceManager.GetLogicalDevice(), memoryManager, frameCount };
}

std::uint32_t RenderEngineVSIndividual::AddModel(
	std::shared_ptr<ModelVS>&& model, const ShaderName& fragmentShader
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
	std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const ShaderName& fragmentShader
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
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
	std::uint64_t frameNumber, const VKSemaphore& imageWaitSemaphore
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
		transferSubmitBuilder
			.SignalSemaphore(transferWaitSemaphore, frameNumber)
			.CommandBuffer(transferCmdBuffer);

		m_transferQueue.SubmitCommandBuffer(transferSubmitBuilder);

		// This should clean the Mesh related temp data when a new model/mesh is added next.
		m_threadPool->SubmitWork(
			std::function{[&modelManager = m_modelManager, &transferWaitSemaphore, frameNumber]
			{
				transferWaitSemaphore.Wait(frameNumber);

				// Should add the other cleanup functions here as well.
				modelManager.CleanUpTempData();
			}}
		);
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
			.WaitSemaphore(transferWaitSemaphore, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, frameNumber)
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

// VS Indirect
RenderEngineVSIndirect::RenderEngineVSIndirect(			// And since the swapchain doesn't support timeline semaphores, it won't be a
			// timeline semaphore.
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount },
	m_computeQueue{
		deviceManager.GetLogicalDevice(),
		deviceManager.GetQueueFamilyManager().GetQueue(QueueType::ComputeQueue),
		deviceManager.GetQueueFamilyManager().GetIndex(QueueType::ComputeQueue)
	}, m_computeWait{}, m_computeDescriptorBuffers{}
{
	// Graphics Descriptors.
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayoutVS(m_graphicsDescriptorBuffers);
	SetCommonGraphicsDescriptorBufferLayout(VK_SHADER_STAGE_VERTEX_BIT);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	if (!std::empty(m_graphicsDescriptorBuffers))
		m_modelManager.CreatePipelineLayout(m_graphicsDescriptorBuffers.front());

	m_cameraManager.CreateBuffer(
		deviceManager.GetQueueFamilyManager().GetComputeAndGraphicsIndices().ResolveQueueIndices(),
		static_cast<std::uint32_t>(frameCount)
	);
	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.SetDescriptorBufferGraphics(m_graphicsDescriptorBuffers, s_cameraBindingSlot);

	// Compute stuffs.
	VkDevice device = deviceManager.GetLogicalDevice();

	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_computeDescriptorBuffers.emplace_back(device, &m_memoryManager);

		// Let's make all of the non graphics semaphores, timeline semaphores.
		m_computeWait.emplace_back(device).Create(true);
	}

	const auto frameCountU32 = static_cast<std::uint32_t>(frameCount);

	m_computeQueue.CreateCommandBuffers(frameCountU32);

	// Compute Descriptors.
	m_modelManager.SetDescriptorBufferLayoutCS(m_computeDescriptorBuffers);
	m_cameraManager.SetDescriptorBufferLayoutCompute(
		m_computeDescriptorBuffers, s_cameraComputeBindingSlot
	);

	for (auto& descriptorBuffer : m_computeDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	if (!std::empty(m_computeDescriptorBuffers))
		m_modelManager.CreatePipelineCS(m_computeDescriptorBuffers.front());

	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.SetDescriptorBufferCompute(m_computeDescriptorBuffers, s_cameraComputeBindingSlot);
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
	std::shared_ptr<ModelVS>&& model, const ShaderName& fragmentShader
) {
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModel(std::move(model), fragmentShader);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBufferVS(m_graphicsDescriptorBuffers);
	m_modelManager.SetDescriptorBufferCSOfModels(m_computeDescriptorBuffers);

	return index;
}

std::uint32_t RenderEngineVSIndirect::AddModelBundle(
	std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const ShaderName& fragmentShader
) {
	// Should wait for the current frames to be rendered before modifying the data.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddModelBundle(std::move(modelBundle), fragmentShader);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBufferVS(m_graphicsDescriptorBuffers);
	m_modelManager.SetDescriptorBufferCSOfModels(m_computeDescriptorBuffers);

	return index;
}

std::uint32_t RenderEngineVSIndirect::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	// Add a mesh Bundle will update the Vertex and Index buffers. So, must wait for the queue to
	// finish.
	m_graphicsQueue.WaitForQueueToFinish();

	const std::uint32_t index = m_modelManager.AddMeshBundle(std::move(meshBundle), m_stagingManager);

	m_modelManager.SetDescriptorBufferCSOfModels(m_computeDescriptorBuffers);

	return index;
}

void RenderEngineVSIndirect::Render(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
	std::uint64_t frameNumber, const VKSemaphore& imageWaitSemaphore
) {
	// Wait for the previous Graphics command buffer to finish.
	m_graphicsQueue.WaitForSubmission(frameIndex);

	Update(static_cast<VkDeviceSize>(frameIndex));

	// Transfer Phase
	const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

		m_stagingManager.Copy(transferCmdBufferScope);

		m_modelManager.ResetCounterBuffer(
			transferCmdBufferScope, static_cast<VkDeviceSize>(frameIndex)
		);

		m_modelManager.CopyTempBuffers(transferCmdBufferScope);

		m_stagingManager.ReleaseOwnership(transferCmdBufferScope, m_transferQueue.GetFamilyIndex());
	}

	const VKSemaphore& transferWaitSemaphore = m_transferWait.at(frameIndex);

	{
		QueueSubmitBuilder<0u, 1u> transferSubmitBuilder{};
		transferSubmitBuilder
			.SignalSemaphore(transferWaitSemaphore, frameNumber)
			.CommandBuffer(transferCmdBuffer);

		m_transferQueue.SubmitCommandBuffer(transferSubmitBuilder);

		// This should clean the Mesh related temp data when a new model/mesh is added next.
		m_threadPool->SubmitWork(
			std::function{[&modelManager = m_modelManager, &transferWaitSemaphore, frameNumber]
			{
				transferWaitSemaphore.Wait(frameNumber);

				// Should add the other cleanup functions here as well.
				modelManager.CleanUpTempData();
			}}
		);
	}

	// Compute Phase
	const VKCommandBuffer& computeCmdBuffer = m_computeQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope computeCmdBufferScope{ computeCmdBuffer };

		m_stagingManager.AcquireOwnership(
			computeCmdBufferScope, m_computeQueue.GetFamilyIndex(), m_transferQueue.GetFamilyIndex()
		);

		VkDescriptorBuffer::BindDescriptorBuffer(
			m_computeDescriptorBuffers.at(frameIndex), computeCmdBufferScope,
			VK_PIPELINE_BIND_POINT_COMPUTE, m_modelManager.GetComputePipelineLayout()
		);

		m_modelManager.Dispatch(computeCmdBufferScope);
	}

	const VKSemaphore& computeWaitSemaphore = m_computeWait.at(frameIndex);

	{
		QueueSubmitBuilder<1u, 1u> computeSubmitBuilder{};
		computeSubmitBuilder
			.SignalSemaphore(computeWaitSemaphore, frameNumber)
			.WaitSemaphore(transferWaitSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, frameNumber)
			.CommandBuffer(computeCmdBuffer);

		m_computeQueue.SubmitCommandBuffer(computeSubmitBuilder);
	}

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
			// The graphics queue should wait for the compute queue to finish and then start the
			// Input Assembler Stage.
			.WaitSemaphore(computeWaitSemaphore, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, frameNumber)
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
