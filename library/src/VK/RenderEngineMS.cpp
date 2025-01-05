#include <RenderEngineMS.hpp>

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
	SetGraphicsDescriptorBufferLayout();

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	ModelManagerMS::SetGraphicsConstantRange(m_graphicsPipelineLayout);

	CreateGraphicsPipelineLayout();

	m_cameraManager.CreateBuffer({}, static_cast<std::uint32_t>(frameCount));
	m_cameraManager.SetDescriptorBufferGraphics(
		m_graphicsDescriptorBuffers, s_cameraBindingSlot, s_vertexShaderSetLayoutIndex
	);

	SetupPipelineStages();
}

void RenderEngineMS::SetGraphicsDescriptorBufferLayout()
{
	// The layout shouldn't change throughout the runtime.
	m_meshManager.SetDescriptorBufferLayout(m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex);
	SetCommonGraphicsDescriptorBufferLayout(VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT);

	for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
	{
		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, s_vertexShaderSetLayoutIndex,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, s_fragmentShaderSetLayoutIndex,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}
}

void RenderEngineMS::SetupPipelineStages()
{
	constexpr size_t stageCount = 2u;

	m_pipelineStages.reserve(stageCount);

	m_pipelineStages.emplace_back(&RenderEngineMS::GenericTransferStage);
	m_pipelineStages.emplace_back(&RenderEngineMS::DrawingStage);
}

void RenderEngineMS::SetModelGraphicsDescriptors()
{
	const auto frameCount = std::size(m_graphicsDescriptorBuffers);

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

std::uint32_t RenderEngineMS::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader
) {
	const std::uint32_t psoIndex = GetGraphicsPSOIndex(fragmentShader);

	const std::uint32_t index    = m_modelManager.AddModelBundle(
		std::move(modelBundle), psoIndex, m_modelBuffers, m_stagingManager, m_temporaryDataBuffer
	);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	SetModelGraphicsDescriptors();

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineMS::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	const std::uint32_t index = m_meshManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_meshManager.SetDescriptorBuffers(m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex);

	m_copyNecessary = true;

	return index;
}

VkSemaphore RenderEngineMS::GenericTransferStage(
	size_t frameIndex,
	[[maybe_unused]] const VKFramebuffer& frameBuffer, [[maybe_unused]] VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	// Transfer Phase

	// If the Transfer stage isn't executed, pass the waitSemaphore on.
	VkSemaphore signalledSemaphore = waitSemaphore;

	// Only execute this stage if copying is necessary.
	if (m_copyNecessary)
	{
		const VKCommandBuffer& transferCmdBuffer = m_transferQueue.GetCommandBuffer(frameIndex);

		{
			const CommandBufferScope transferCmdBufferScope{ transferCmdBuffer };

			// Need to copy the old buffers first to avoid empty data being copied over
			// the queued data.
			m_meshManager.CopyOldBuffers(transferCmdBuffer);
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

VkSemaphore RenderEngineMS::DrawingStage(
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
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout
		);

		BeginRenderPass(graphicsCmdBufferScope, frameBuffer, renderArea);

		m_modelManager.Draw(graphicsCmdBufferScope, m_meshManager, m_graphicsPipelineManager);

		m_renderPass.EndPass(graphicsCmdBuffer.Get());
	}

	const VKSemaphore& graphicsWaitSemaphore = m_graphicsWait[frameIndex];

	{
		const std::uint64_t oldSemaphoreCounterValue = semaphoreCounter;
		++semaphoreCounter;

		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalSemaphore(graphicsWaitSemaphore)
			.WaitSemaphore(waitSemaphore, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT, oldSemaphoreCounterValue)
			.CommandBuffer(graphicsCmdBuffer);

		VKFence& signalFence = m_graphicsQueue.GetFence(frameIndex);
		signalFence.Reset();

		m_graphicsQueue.SubmitCommandBuffer(graphicsSubmitBuilder, signalFence);
	}

	return graphicsWaitSemaphore.Get();
}

ModelManagerMS RenderEngineMS::GetModelManager(
	[[maybe_unused]] const VkDeviceManager& deviceManager,
	MemoryManager* memoryManager,
	[[maybe_unused]] std::uint32_t frameCount
) {
	return ModelManagerMS{ memoryManager };
}

ModelBuffers RenderEngineMS::ConstructModelBuffers(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager, std::uint32_t frameCount
) {
	// Will be accessed from both the Graphics queue only.
	return ModelBuffers{ deviceManager.GetLogicalDevice(), memoryManager, frameCount, {}};
}
