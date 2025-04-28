#include <VkRenderEngineMS.hpp>

namespace Terra
{
void RenderEngineMSDeviceExtension::SetDeviceExtensions(
	VkDeviceExtensionManager& extensionManager
) noexcept {
	RenderEngineDeviceExtension::SetDeviceExtensions(extensionManager);

	extensionManager.AddExtensions(PipelineModelsMSIndividual::GetRequiredExtensions());
}

RenderEngineMS::RenderEngineMS(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	m_modelManager = std::make_unique<ModelManagerMS>();

	SetGraphicsDescriptorBufferLayout();

	m_cameraManager.CreateBuffer({}, static_cast<std::uint32_t>(frameCount));
}

void RenderEngineMS::FinaliseInitialisation()
{
	m_externalResourceManager.SetGraphicsDescriptorLayout(m_graphicsDescriptorBuffers);

	for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();

	ModelManagerMS::SetGraphicsConstantRange(m_graphicsPipelineLayout);

	CreateGraphicsPipelineLayout();

	m_cameraManager.SetDescriptorBufferGraphics(
		m_graphicsDescriptorBuffers, s_cameraBindingSlot, s_vertexShaderSetLayoutIndex
	);
}

void RenderEngineMS::SetGraphicsDescriptorBufferLayout()
{
	// The layout shouldn't change throughout the runtime.
	m_meshManager.SetDescriptorBufferLayout(
		m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex
	);
	SetCommonGraphicsDescriptorBufferLayout(
		VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT
	);

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

VkSemaphore RenderEngineMS::ExecutePipelineStages(
	size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	waitSemaphore = GenericTransferStage(frameIndex, semaphoreCounter, waitSemaphore);

	waitSemaphore = DrawingStage(
		frameIndex, renderTarget, renderArea, semaphoreCounter, waitSemaphore
	);

	return waitSemaphore;
}

void RenderEngineMS::SetModelGraphicsDescriptors()
{
	const size_t frameCount = std::size(m_graphicsDescriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = m_graphicsDescriptorBuffers[index];
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot,
			s_vertexShaderSetLayoutIndex
		);
		m_modelBuffers.SetFragmentDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot,
			s_fragmentShaderSetLayoutIndex
		);
	}
}

std::uint32_t RenderEngineMS::AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
{
	std::vector<std::uint32_t> modelBufferIndices = AddModelsToBuffer(
		*modelBundle, m_modelBuffers
	);

	const std::uint32_t index = m_modelManager->AddModelBundle(
		std::move(modelBundle), modelBufferIndices
	);

	// After a new model has been added, the ModelBuffer might get recreated. So, it will have
	// a new object. So, we should set that new object as the descriptor.
	SetModelGraphicsDescriptors();

	m_copyNecessary = true;

	return index;
}

std::uint32_t RenderEngineMS::AddMeshBundle(MeshBundleTemporaryData&& meshBundle)
{
	const std::uint32_t index = m_meshManager.AddMeshBundle(
		std::move(meshBundle), m_stagingManager, m_temporaryDataBuffer
	);

	m_meshManager.SetDescriptorBuffers(m_graphicsDescriptorBuffers, s_vertexShaderSetLayoutIndex);

	m_copyNecessary = true;

	return index;
}

VkSemaphore RenderEngineMS::GenericTransferStage(
	size_t frameIndex, std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
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
			m_externalResourceManager.CopyQueuedBuffers(transferCmdBufferScope);
			m_meshManager.CopyOldBuffers(transferCmdBufferScope);
			m_stagingManager.CopyAndClearQueuedBuffers(transferCmdBufferScope);

			m_stagingManager.ReleaseOwnership(
				transferCmdBufferScope, m_transferQueue.GetFamilyIndex()
			);
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

void RenderEngineMS::DrawRenderPassPipelines(
	const VKCommandBuffer& graphicsCmdBuffer, const ExternalRenderPass_t& renderPass
) const noexcept {
	const std::vector<VkExternalRenderPass::PipelineDetails>& pipelineDetails
		= renderPass.GetPipelineDetails();

	for (const VkExternalRenderPass::PipelineDetails& details : pipelineDetails)
	{
		const std::vector<std::uint32_t>& bundleIndices        = details.modelBundleIndices;
		const std::vector<std::uint32_t>& pipelineLocalIndices = details.pipelineLocalIndices;

		m_graphicsPipelineManager.BindPipeline(details.pipelineGlobalIndex, graphicsCmdBuffer);

		const size_t bundleCount = std::size(bundleIndices);

		for (size_t index = 0u; index < bundleCount; ++index)
			m_modelManager->DrawPipeline(
				bundleIndices[index], pipelineLocalIndices[index],
				graphicsCmdBuffer, m_meshManager, m_graphicsPipelineLayout.Get()
			);
	}
}

VkSemaphore RenderEngineMS::DrawingStage(
	size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
	std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
) {
	// Graphics Phase
	const VKCommandBuffer& graphicsCmdBuffer = m_graphicsQueue.GetCommandBuffer(frameIndex);

	{
		const CommandBufferScope graphicsCmdBufferScope{ graphicsCmdBuffer };

		m_stagingManager.AcquireOwnership(
			graphicsCmdBufferScope, m_graphicsQueue.GetFamilyIndex(),
			m_transferQueue.GetFamilyIndex()
		);

		m_textureStorage.TransitionQueuedTextures(graphicsCmdBufferScope);

		m_viewportAndScissors.BindViewportAndScissor(graphicsCmdBufferScope);

		VkDescriptorBuffer::BindDescriptorBuffer(
			m_graphicsDescriptorBuffers[frameIndex], graphicsCmdBufferScope,
			VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout
		);

		// Normal passes
		const size_t renderPassCount = std::size(m_renderPasses);

		for (size_t index = 0u; index < renderPassCount; ++index)
		{
			if (!m_renderPasses.IsInUse(index))
				continue;

			const ExternalRenderPass_t& renderPass = *m_renderPasses[index];

			renderPass.StartPass(graphicsCmdBufferScope, renderArea);

			DrawRenderPassPipelines(graphicsCmdBufferScope, renderPass);

			renderPass.EndPass(graphicsCmdBufferScope);
		}

		// The one for the swapchain
		if (m_swapchainRenderPass)
		{
			const ExternalRenderPass_t& renderPass = *m_swapchainRenderPass;

			renderPass.StartPass(graphicsCmdBufferScope, renderArea);

			DrawRenderPassPipelines(graphicsCmdBufferScope, renderPass);

			renderPass.EndPassForSwapchain(graphicsCmdBufferScope, renderTarget);
		}
	}

	const VKSemaphore& graphicsWaitSemaphore = m_graphicsWait[frameIndex];

	{
		const std::uint64_t oldSemaphoreCounterValue = semaphoreCounter;
		++semaphoreCounter;

		QueueSubmitBuilder<1u, 1u> graphicsSubmitBuilder{};
		graphicsSubmitBuilder
			.SignalSemaphore(graphicsWaitSemaphore)
			.WaitSemaphore(
				waitSemaphore, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT, oldSemaphoreCounterValue
			).CommandBuffer(graphicsCmdBuffer);

		VKFence& signalFence = m_graphicsQueue.GetFence(frameIndex);
		signalFence.Reset();

		m_graphicsQueue.SubmitCommandBuffer(graphicsSubmitBuilder, signalFence);
	}

	return graphicsWaitSemaphore.Get();
}
}
