#include <RenderEngineVS.hpp>

// VS Individual
RenderEngineVSIndividual::RenderEngineVSIndividual(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	SetGraphicsDescriptorBufferLayout();

	m_cameraManager.CreateBuffer({}, static_cast<std::uint32_t>(frameCount));
}

void RenderEngineVSIndividual::FinaliseInitialisation()
{
	m_externalResourceManager.SetGraphicsDescriptorLayout(m_graphicsDescriptorBuffers);

	for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	ModelManagerVSIndividual::SetGraphicsConstantRange(m_graphicsPipelineLayout);

	CreateGraphicsPipelineLayout();

	m_cameraManager.SetDescriptorBufferGraphics(
		m_graphicsDescriptorBuffers, s_cameraBindingSlot, s_vertexShaderSetLayoutIndex
	);
}

VkSemaphore RenderEngineVSIndividual::ExecutePipelineStages(
	size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	waitSemaphore = GenericTransferStage(frameIndex, semaphoreCounter, waitSemaphore);

	waitSemaphore = DrawingStage(frameIndex, renderTarget, renderArea, semaphoreCounter, waitSemaphore);

	return waitSemaphore;
}

void RenderEngineVSIndividual::SetGraphicsDescriptorBufferLayout()
{
	// The layout shouldn't change throughout the runtime.
	SetCommonGraphicsDescriptorBufferLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
	{
		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, s_vertexShaderSetLayoutIndex,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, s_fragmentShaderSetLayoutIndex,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}
}

ModelManagerVSIndividual RenderEngineVSIndividual::GetModelManager(
	[[maybe_unused]] const VkDeviceManager& deviceManager,
	MemoryManager* memoryManager,
	[[maybe_unused]] std::uint32_t frameCount
) {
	return ModelManagerVSIndividual{ memoryManager };
}

void RenderEngineVSIndividual::SetGraphicsDescriptors()
{
	const size_t frameCount = std::size(m_graphicsDescriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = m_graphicsDescriptorBuffers[index];
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot, s_vertexShaderSetLayoutIndex
		);
		m_modelBuffers.SetFragmentDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot, s_fragmentShaderSetLayoutIndex
		);
	}
}

ModelBuffers RenderEngineVSIndividual::ConstructModelBuffers(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager, std::uint32_t frameCount
) {
	// Only being accessed from the graphics queue.
	return ModelBuffers{ deviceManager.GetLogicalDevice(), memoryManager, frameCount, {}};
}

std::uint32_t RenderEngineVSIndividual::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader
) {
	const std::uint32_t psoIndex = m_renderPassManager.AddOrGetGraphicsPipeline(fragmentShader);

	const std::uint32_t index    = m_modelManager.AddModelBundle(
		std::move(modelBundle), psoIndex, m_modelBuffers
	);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	SetGraphicsDescriptors();

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineVSIndividual::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	m_copyNecessary = true;

	return m_meshManager.AddMeshBundle(std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer);
}

VkSemaphore RenderEngineVSIndividual::GenericTransferStage(
	size_t frameIndex, std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	// Transfer Phase

	// If the Transfer stage isn't executed, pass the waitSemaphore on.
	VkSemaphore signalledSemaphore = waitSemaphore;

	if (m_copyNecessary)
	{
		const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

		{
			const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

			// Need to copy the old buffers first to avoid empty data being copied over
			// the queued data.
			m_externalResourceManager.CopyQueuedBuffers(transferCmdBufferScope);
			m_meshManager.CopyOldBuffers(transferCmdBufferScope);
			m_stagingManager.CopyAndClearQueuedBuffers(transferCmdBufferScope);

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
	size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
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
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout
		);

		m_renderPassManager.BeginRenderingWithDepth(
			graphicsCmdBufferScope, renderArea, renderTarget, m_backgroundColour
		);

		m_modelManager.Draw(
			graphicsCmdBufferScope, m_meshManager, m_renderPassManager.GetGraphicsPipelineManager()
		);

		m_renderPassManager.EndRendering(graphicsCmdBufferScope, renderTarget);
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
	}, m_computeWait{}, m_computeDescriptorBuffers{},
	m_computePipelineManager{ deviceManager.GetLogicalDevice() },
	m_computePipelineLayout{ deviceManager.GetLogicalDevice() }
{
	// Graphics Descriptors.
	// The layout shouldn't change throughout the runtime.
	SetGraphicsDescriptorBufferLayout();

	m_cameraManager.CreateBuffer(
		deviceManager.GetQueueFamilyManager().GetComputeAndGraphicsIndices().ResolveQueueIndices(),
		static_cast<std::uint32_t>(frameCount)
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
	SetComputeDescriptorBufferLayout();
}

void RenderEngineVSIndirect::CreateComputePipelineLayout()
{
	if (!std::empty(m_computeDescriptorBuffers))
		m_computePipelineLayout.Create(m_computeDescriptorBuffers.front().GetValidLayouts());

	m_computePipelineManager.SetPipelineLayout(m_computePipelineLayout.Get());
}

void RenderEngineVSIndirect::FinaliseInitialisation()
{
	m_externalResourceManager.SetGraphicsDescriptorLayout(m_graphicsDescriptorBuffers);

	// Graphics
	for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	ModelManagerVSIndirect::SetGraphicsConstantRange(m_graphicsPipelineLayout);

	CreateGraphicsPipelineLayout();

	m_cameraManager.SetDescriptorBufferGraphics(
		m_graphicsDescriptorBuffers, s_cameraBindingSlot, s_vertexShaderSetLayoutIndex
	);

	// Compute
	for (VkDescriptorBuffer& descriptorBuffer : m_computeDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	ModelManagerVSIndirect::SetComputeConstantRange(m_computePipelineLayout);

	CreateComputePipelineLayout();

	m_cameraManager.SetDescriptorBufferCompute(
		m_computeDescriptorBuffers, s_cameraComputeBindingSlot, s_computeShaderSetLayoutIndex
	);

	assert(
		!std::empty(m_computePipelineManager.GetShaderPath())
		&& "The shader path should be set before calling this function."
	);
	// Add the Frustum Culling Shader.
	m_computePipelineManager.AddOrGetComputePipeline(L"VertexShaderCSIndirect");
}

void RenderEngineVSIndirect::SetGraphicsDescriptorBufferLayout()
{
	// The layout shouldn't change throughout the runtime.
	SetCommonGraphicsDescriptorBufferLayout(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	m_modelManager.SetDescriptorBufferLayoutVS(m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex);

	for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
	{
		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, s_vertexShaderSetLayoutIndex,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, s_fragmentShaderSetLayoutIndex,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}
}

void RenderEngineVSIndirect::SetComputeDescriptorBufferLayout()
{
	m_modelManager.SetDescriptorBufferLayoutCS(m_computeDescriptorBuffers, s_computeShaderSetLayoutIndex);
	m_meshManager.SetDescriptorBufferLayoutCS(m_computeDescriptorBuffers, s_computeShaderSetLayoutIndex);

	m_cameraManager.SetDescriptorBufferLayoutCompute(
		m_computeDescriptorBuffers, s_cameraComputeBindingSlot, s_computeShaderSetLayoutIndex
	);

	for (VkDescriptorBuffer& descriptorBuffer : m_computeDescriptorBuffers)
		descriptorBuffer.AddBinding(
			s_modelBuffersComputeBindingSlot, s_computeShaderSetLayoutIndex,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_COMPUTE_BIT
		);
}

VkSemaphore RenderEngineVSIndirect::ExecutePipelineStages(
	size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	waitSemaphore = GenericTransferStage(frameIndex, semaphoreCounter, waitSemaphore);

	waitSemaphore = FrustumCullingStage(frameIndex, semaphoreCounter, waitSemaphore);

	waitSemaphore = DrawingStage(frameIndex, renderTarget, renderArea, semaphoreCounter, waitSemaphore);

	return waitSemaphore;
}

ModelManagerVSIndirect RenderEngineVSIndirect::GetModelManager(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager, std::uint32_t frameCount
) {
	return ModelManagerVSIndirect
	{
		deviceManager.GetLogicalDevice(), memoryManager,
		deviceManager.GetQueueFamilyManager().GetAllIndices(), frameCount
	};
}

ModelBuffers RenderEngineVSIndirect::ConstructModelBuffers(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager, std::uint32_t frameCount
) {
	// Will be accessed from both the Graphics queue and the compute queue.
	return ModelBuffers
	{
		deviceManager.GetLogicalDevice(), memoryManager, frameCount,
		deviceManager.GetQueueFamilyManager().GetComputeAndGraphicsIndices().ResolveQueueIndices()
	};
}

void RenderEngineVSIndirect::SetModelGraphicsDescriptors()
{
	m_modelManager.SetDescriptorBuffersVS(
		m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex
	);

	const size_t frameCount = std::size(m_graphicsDescriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = m_graphicsDescriptorBuffers[index];
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot, s_vertexShaderSetLayoutIndex
		);
		m_modelBuffers.SetFragmentDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot, s_fragmentShaderSetLayoutIndex
		);
	}
}

void RenderEngineVSIndirect::SetModelComputeDescriptors()
{
	m_modelManager.SetDescriptorBuffersCS(m_computeDescriptorBuffers, s_computeShaderSetLayoutIndex);

	const size_t frameCount = std::size(m_computeDescriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = m_computeDescriptorBuffers[index];
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersComputeBindingSlot, s_computeShaderSetLayoutIndex
		);
	}
}

void RenderEngineVSIndirect::_updatePerFrame(VkDeviceSize frameIndex) const noexcept
{
	m_modelManager.UpdatePerFrame(frameIndex, m_meshManager);
}

void RenderEngineVSIndirect::SetShaderPath(const std::wstring& shaderPath)
{
	RenderEngineCommon::SetShaderPath(shaderPath);

	m_computePipelineManager.SetShaderPath(shaderPath);
}

std::uint32_t RenderEngineVSIndirect::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader
) {
	const std::uint32_t psoIndex = m_renderPassManager.AddOrGetGraphicsPipeline(fragmentShader);

	const std::uint32_t index    = m_modelManager.AddModelBundle(
		std::move(modelBundle), psoIndex, m_modelBuffers
	);

	// After new models have been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	SetModelGraphicsDescriptors();
	SetModelComputeDescriptors();

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineVSIndirect::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	const std::uint32_t index = m_meshManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_meshManager.SetDescriptorBuffersCS(m_computeDescriptorBuffers, s_computeShaderSetLayoutIndex);

	m_copyNecessary = true;

	return index;
}

VkSemaphore RenderEngineVSIndirect::GenericTransferStage(
	size_t frameIndex, std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	// Transfer Phase

	// If the Transfer stage isn't executed, pass the waitSemaphore on.
	VkSemaphore signalledSemaphore = waitSemaphore;

	if (m_copyNecessary)
	{
		const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

		{
			const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

			// Need to copy the old buffers first to avoid empty data being copied over
			// the queued data.
			m_externalResourceManager.CopyQueuedBuffers(transferCmdBufferScope);
			m_meshManager.CopyOldBuffers(transferCmdBufferScope);
			m_stagingManager.CopyAndClearQueuedBuffers(transferCmdBufferScope);

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
	size_t frameIndex, std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
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
			VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout
		);

		m_modelManager.Dispatch(computeCmdBufferScope, m_computePipelineManager);
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
	size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
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
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout
		);

		m_renderPassManager.BeginRenderingWithDepth(
			graphicsCmdBufferScope, renderArea, renderTarget, m_backgroundColour
		);

		m_modelManager.Draw(
			frameIndex, graphicsCmdBufferScope, m_meshManager,
			m_renderPassManager.GetGraphicsPipelineManager()
		);

		m_renderPassManager.EndRendering(graphicsCmdBufferScope, renderTarget);
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
