#include <RenderEngineVertexShader.hpp>

// VS Individual
RenderEngineVSIndividual::RenderEngineVSIndividual(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayout(
		m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex, s_fragmentShaderSetLayoutIndex
	);
	SetCommonGraphicsDescriptorBufferLayout(VK_SHADER_STAGE_VERTEX_BIT);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	if (!std::empty(m_graphicsDescriptorBuffers))
		m_modelManager.CreatePipelineLayout(m_graphicsDescriptorBuffers.front());

	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.CreateBuffer({}, static_cast<std::uint32_t>(frameCount));
	m_cameraManager.SetDescriptorBufferGraphics(
		m_graphicsDescriptorBuffers, s_cameraBindingSlot, s_vertexShaderSetLayoutIndex
	);

	SetupPipelineStages();
}

void RenderEngineVSIndividual::SetupPipelineStages()
{
	constexpr size_t stageCount = 2u;

	m_pipelineStages.reserve(stageCount);

	m_pipelineStages.emplace_back(&RenderEngineVSIndividual::GenericTransferStage);
	m_pipelineStages.emplace_back(&RenderEngineVSIndividual::DrawingStage);
}

ModelManagerVSIndividual RenderEngineVSIndividual::GetModelManager(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
	[[maybe_unused]] StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) {
	return ModelManagerVSIndividual{ deviceManager.GetLogicalDevice(), memoryManager, frameCount };
}

std::uint32_t RenderEngineVSIndividual::AddModelBundle(
	std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& fragmentShader
) {
	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), fragmentShader, m_temporaryDataBuffer
	);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBuffer(
		m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex, s_fragmentShaderSetLayoutIndex
	);

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineVSIndividual::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	m_copyNecessary = true;

	return m_modelManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);
}

VkSemaphore RenderEngineVSIndividual::GenericTransferStage(
	size_t frameIndex,
	[[maybe_unused]] const VKFramebuffer& frameBuffer, [[maybe_unused]] VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	// Transfer Phase

	// If the Transfer stage isn't executed, pass the waitSemaphore on.
	VkSemaphore signalledSemaphore = waitSemaphore;

	if (m_copyNecessary)
	{
		const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

		{
			const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

			m_stagingManager.CopyAndClear(transferCmdBufferScope);
			m_modelManager.CopyTempData(transferCmdBufferScope);

			m_stagingManager.ReleaseOwnership(transferCmdBufferScope, m_transferQueue.GetFamilyIndex());
		}

		const VKSemaphore& transferWaitSemaphore = m_transferWait[frameIndex];

		{
			QueueSubmitBuilder<1u, 1u> transferSubmitBuilder{};
			transferSubmitBuilder
				.SignalSemaphore(transferWaitSemaphore, semaphoreCounter)
				.WaitSemaphore(waitSemaphore, VK_PIPELINE_STAGE_TRANSFER_BIT)
				.CommandBuffer(transferCmdBuffer);

			m_transferQueue.SubmitCommandBuffer(transferSubmitBuilder);

			m_temporaryDataBuffer.SetUsed(frameIndex);
		}

		// Since this is the first stage for now. The receiving semaphore won't be a timeline one.
		// So, no need to increase it.

		m_copyNecessary    = false;
		signalledSemaphore = transferWaitSemaphore.Get();
	}

	return signalledSemaphore;
}

VkSemaphore RenderEngineVSIndividual::DrawingStage(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
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
		const std::uint64_t oldSemaphoreCounterValue = semaphoreCounter;
		++semaphoreCounter;

		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalSemaphore(graphicsWaitSemaphore)
			.WaitSemaphore(waitSemaphore, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, oldSemaphoreCounterValue)
			.CommandBuffer(graphicsCmdBuffer);

		VKFence& signalFence = m_graphicsQueue.GetFence(frameIndex);
		signalFence.Reset();

		m_graphicsQueue.SubmitCommandBuffer(graphicsSubmitBuilder, signalFence);
	}

	return graphicsWaitSemaphore.Get();
}

// VS Indirect
RenderEngineVSIndirect::RenderEngineVSIndirect(
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
	m_modelManager.SetDescriptorBufferLayoutVS(
		m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex, s_fragmentShaderSetLayoutIndex
	);
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
	m_cameraManager.SetDescriptorBufferGraphics(
		m_graphicsDescriptorBuffers, s_cameraBindingSlot, s_vertexShaderSetLayoutIndex
	);

	// Compute stuffs.
	VkDevice device = deviceManager.GetLogicalDevice();

	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_computeDescriptorBuffers.emplace_back(device, &m_memoryManager, s_computePipelineSetLayoutCount);

		// Let's make all of the non graphics semaphores, timeline semaphores.
		m_computeWait.emplace_back(device).Create(true);
	}

	const auto frameCountU32 = static_cast<std::uint32_t>(frameCount);

	m_computeQueue.CreateCommandBuffers(frameCountU32);

	// Compute Descriptors.
	m_modelManager.SetDescriptorBufferLayoutCS(m_computeDescriptorBuffers, s_computeShaderSetLayoutIndex);
	m_cameraManager.SetDescriptorBufferLayoutCompute(
		m_computeDescriptorBuffers, s_cameraComputeBindingSlot, s_computeShaderSetLayoutIndex
	);

	for (auto& descriptorBuffer : m_computeDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	if (!std::empty(m_computeDescriptorBuffers))
		m_modelManager.CreatePipelineCS(m_computeDescriptorBuffers.front());

	// This descriptor shouldn't change, so it should be fine to set it here.
	m_cameraManager.SetDescriptorBufferCompute(
		m_computeDescriptorBuffers, s_cameraComputeBindingSlot, s_computeShaderSetLayoutIndex
	);

	SetupPipelineStages();
}

void RenderEngineVSIndirect::SetupPipelineStages()
{
	constexpr size_t stageCount = 3u;

	m_pipelineStages.reserve(stageCount);

	m_pipelineStages.emplace_back(&RenderEngineVSIndirect::GenericTransferStage);
	m_pipelineStages.emplace_back(&RenderEngineVSIndirect::FrustumCullingStage);
	m_pipelineStages.emplace_back(&RenderEngineVSIndirect::DrawingStage);
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

std::uint32_t RenderEngineVSIndirect::AddModelBundle(
	std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& fragmentShader
) {
	const std::uint32_t index = m_modelManager.AddModelBundle(
		std::move(modelBundle), fragmentShader, m_temporaryDataBuffer
	);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	m_modelManager.SetDescriptorBufferVS(
		m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex, s_fragmentShaderSetLayoutIndex
	);
	m_modelManager.SetDescriptorBufferCSOfModels(m_computeDescriptorBuffers, s_computeShaderSetLayoutIndex);

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineVSIndirect::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	const std::uint32_t index = m_modelManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_modelManager.SetDescriptorBufferCSOfMeshes(m_computeDescriptorBuffers, s_computeShaderSetLayoutIndex);

	m_copyNecessary = true;

	return index;
}

VkSemaphore RenderEngineVSIndirect::GenericTransferStage(
	size_t frameIndex,
	[[maybe_unused]] const VKFramebuffer& frameBuffer, [[maybe_unused]] VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	// Transfer Phase

	// If the Transfer stage isn't executed, pass the waitSemaphore on.
	VkSemaphore signalledSemaphore = waitSemaphore;

	if (m_copyNecessary)
	{
		const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

		{
			const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

			m_stagingManager.CopyAndClear(transferCmdBufferScope);
			m_modelManager.CopyTempBuffers(transferCmdBufferScope);

			m_stagingManager.ReleaseOwnership(transferCmdBufferScope, m_transferQueue.GetFamilyIndex());
		}

		const VKSemaphore& transferWaitSemaphore = m_transferWait[frameIndex];

		{
			QueueSubmitBuilder<1u, 1u> transferSubmitBuilder{};
			transferSubmitBuilder
				.SignalSemaphore(transferWaitSemaphore, semaphoreCounter)
				.WaitSemaphore(waitSemaphore, VK_PIPELINE_STAGE_TRANSFER_BIT)
				.CommandBuffer(transferCmdBuffer);

			m_transferQueue.SubmitCommandBuffer(transferSubmitBuilder);

			m_temporaryDataBuffer.SetUsed(frameIndex);
		}

		// Since this is the first stage for now. The receiving semaphore won't be a timeline one.
		// So, no need to increase it.

		m_copyNecessary    = false;
		signalledSemaphore = transferWaitSemaphore.Get();
	}

	return signalledSemaphore;
}

VkSemaphore RenderEngineVSIndirect::FrustumCullingStage(
	size_t frameIndex,
	[[maybe_unused]] const VKFramebuffer& frameBuffer, [[maybe_unused]] VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
// Compute Phase
	const VKCommandBuffer& computeCmdBuffer = m_computeQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope computeCmdBufferScope{ computeCmdBuffer };

		m_stagingManager.AcquireOwnership(
			computeCmdBufferScope, m_computeQueue.GetFamilyIndex(), m_transferQueue.GetFamilyIndex()
		);

		m_modelManager.ResetCounterBuffer(computeCmdBuffer, static_cast<VkDeviceSize>(frameIndex));

		VkDescriptorBuffer::BindDescriptorBuffer(
			m_computeDescriptorBuffers[frameIndex], computeCmdBufferScope,
			VK_PIPELINE_BIND_POINT_COMPUTE, m_modelManager.GetComputePipelineLayout()
		);

		m_modelManager.Dispatch(computeCmdBufferScope);
	}

	const VKSemaphore& computeWaitSemaphore = m_computeWait[frameIndex];

	{
		const std::uint64_t oldSemaphoreCounterValue = semaphoreCounter;
		++semaphoreCounter;

		QueueSubmitBuilder<1u, 1u> computeSubmitBuilder{};
		computeSubmitBuilder
			.SignalSemaphore(computeWaitSemaphore, semaphoreCounter)
			.WaitSemaphore(waitSemaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, oldSemaphoreCounterValue)
			.CommandBuffer(computeCmdBuffer);

		m_computeQueue.SubmitCommandBuffer(computeSubmitBuilder);
	}

	return computeWaitSemaphore.Get();
}

VkSemaphore RenderEngineVSIndirect::DrawingStage(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
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

		m_modelManager.Draw(frameIndex, graphicsCmdBufferScope);

		m_renderPass.EndPass(graphicsCmdBuffer.Get());
	}

	const VKSemaphore& graphicsWaitSemaphore = m_graphicsWait[frameIndex];

	{
		const std::uint64_t oldSemaphoreCounterValue = semaphoreCounter;
		++semaphoreCounter;

		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalSemaphore(graphicsWaitSemaphore)
			.WaitSemaphore(waitSemaphore, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, oldSemaphoreCounterValue)
			.CommandBuffer(graphicsCmdBuffer);

		VKFence& signalFence = m_graphicsQueue.GetFence(frameIndex);
		signalFence.Reset();

		m_graphicsQueue.SubmitCommandBuffer(graphicsSubmitBuilder, signalFence);
	}

	return graphicsWaitSemaphore.Get();
}
