#include <RenderEngine.hpp>

void RenderEngineDeviceExtension::SetDeviceExtensions(
	VkDeviceExtensionManager& extensionManager
) noexcept {
	extensionManager.AddExtensions(VkDescriptorBuffer::GetRequiredExtensions());
	extensionManager.AddExtensions(MemoryManager::GetRequiredExtensions());
}

RenderEngine::RenderEngine(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngine {
		deviceManager.GetPhysicalDevice(),
		deviceManager.GetLogicalDevice(),
		deviceManager.GetQueueFamilyManagerRef(),
		std::move(threadPool), frameCount
	}
{}

RenderEngine::RenderEngine(
	VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
	VkQueueFamilyMananger const* queueFamilyManager, std::shared_ptr<ThreadPool> threadPool,
	size_t frameCount
) : m_threadPool{ std::move(threadPool) },
	m_memoryManager{ physicalDevice, logicalDevice, 20_MB, 400_KB },
	m_graphicsQueue{
		logicalDevice,
		queueFamilyManager->GetQueue(QueueType::GraphicsQueue),
		queueFamilyManager->GetIndex(QueueType::GraphicsQueue)
	}, m_graphicsWait{},
	m_transferQueue{
		logicalDevice,
		queueFamilyManager->GetQueue(QueueType::TransferQueue),
		queueFamilyManager->GetIndex(QueueType::TransferQueue)
	}, m_transferWait{},
	m_stagingManager{ logicalDevice, &m_memoryManager, m_threadPool.get(), queueFamilyManager },
	m_externalResourceManager{ logicalDevice, &m_memoryManager },
	m_graphicsDescriptorBuffers{},
	m_graphicsPipelineLayout{ logicalDevice },
	m_textureStorage{ logicalDevice, &m_memoryManager },
	m_textureManager{ logicalDevice, &m_memoryManager },
	m_cameraManager{ logicalDevice, &m_memoryManager },
	m_backgroundColour{ { 0.0001f, 0.0001f, 0.0001f, 0.0001f } }, m_viewportAndScissors{},
	m_temporaryDataBuffer{}, m_copyNecessary{ false }
{
	VkDescriptorBuffer::SetDescriptorBufferInfo(physicalDevice);

	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_graphicsDescriptorBuffers.emplace_back(
			logicalDevice, &m_memoryManager, s_graphicsPipelineSetLayoutCount
		);

		// The graphics Wait semaphores will be used by the Swapchain, which doesn't support
		// timeline semaphores.
		m_graphicsWait.emplace_back(logicalDevice).Create(false);
		// Setting the Transfer ones to timeline, as we would want to clean the tempData
		// on a different thread when the transfer queue is finished.
		m_transferWait.emplace_back(logicalDevice).Create(true);
	}

	const auto frameCountU32 = static_cast<std::uint32_t>(frameCount);

	m_graphicsQueue.CreateCommandBuffers(frameCountU32);
	m_transferQueue.CreateCommandBuffers(frameCountU32);
}

void RenderEngine::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept
{
	m_backgroundColour = {
		{ colourVector.at(0u), colourVector.at(1), colourVector.at(2), colourVector.at(3) }
	};
}

size_t RenderEngine::AddTextureAsCombined(STexture&& texture)
{
	const size_t textureIndex = m_textureStorage.AddTexture(
		std::move(texture), m_stagingManager, m_temporaryDataBuffer
	);

	m_copyNecessary = true;

	return textureIndex;
}

void RenderEngine::UnbindCombinedTexture(size_t index)
{
	UnbindCombinedTexture(index, m_textureStorage.GetDefaultSamplerIndex());
}

void RenderEngine::UnbindCombinedTexture(size_t textureIndex, size_t samplerIndex)
{
	// This function shouldn't need to wait for the GPU to finish, as it isn't doing
	// anything on the GPU side.
	static constexpr VkDescriptorType DescType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	const std::uint32_t globalDescIndex = m_textureStorage.GetTextureBindingIndex(textureIndex);

	if (!std::empty(m_graphicsDescriptorBuffers))
	{
		void const* globalDescriptor = m_graphicsDescriptorBuffers.front().GetDescriptor<DescType>(
			s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex, globalDescIndex
		);

		m_textureManager.SetLocalDescriptor<DescType>(
			globalDescriptor,
			TextureManager::DescDetailsCombined{
				.textureIndex = static_cast<std::uint32_t>(textureIndex),
				.samplerIndex = static_cast<std::uint32_t>(samplerIndex)
			}
		);

		m_textureManager.SetAvailableIndex<DescType>(globalDescIndex, true);
	}
}

std::uint32_t RenderEngine::BindCombinedTexture(size_t index)
{
	return BindCombinedTexture(index, m_textureStorage.GetDefaultSamplerIndex());
}

std::uint32_t RenderEngine::BindCombinedTexture(size_t textureIndex, size_t samplerIndex)
{
	static constexpr VkDescriptorType DescType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	static constexpr TextureDescType TexDescType = TextureDescType::CombinedTexture;

	VkTextureView const* textureView = m_textureStorage.GetPtr(textureIndex);
	VKSampler const* defaultSampler  = m_textureStorage.GetSamplerPtr(samplerIndex);

	auto oFreeGlobalDescIndex        = m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

	// If there is no free global index, increase the limit. Right now it should be possible to
	// have 65535 bound textures at once. There could be more textures.
	if (!oFreeGlobalDescIndex)
	{
		m_textureManager.IncreaseAvailableIndices<TexDescType>();

		for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
		{
			const std::vector<VkDescriptorSetLayoutBinding> oldSetLayoutBindings
				= descriptorBuffer.GetLayout(s_fragmentShaderSetLayoutIndex).GetBindings();

			m_textureManager.SetDescriptorBufferLayout(
				descriptorBuffer, s_combinedTextureBindingSlot, s_sampledTextureBindingSlot,
				s_samplerBindingSlot, s_fragmentShaderSetLayoutIndex
			);

			descriptorBuffer.RecreateSetLayout(s_fragmentShaderSetLayoutIndex, oldSetLayoutBindings);
		}

		oFreeGlobalDescIndex = m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

		ResetGraphicsPipeline();
	}

	const std::uint32_t freeGlobalDescIndex = oFreeGlobalDescIndex.value();

	m_textureManager.SetAvailableIndex<DescType>(freeGlobalDescIndex, false);
	m_textureStorage.SetTextureBindingIndex(textureIndex, freeGlobalDescIndex);

	auto localDesc = m_textureManager.GetLocalDescriptor<DescType>(
		TextureManager::DescDetailsCombined{
			.textureIndex = static_cast<std::uint32_t>(textureIndex),
			.samplerIndex = static_cast<std::uint32_t>(samplerIndex)
		}
	);

	if (localDesc)
	{
		void const* localDescriptor = localDesc.value();

		for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
			descriptorBuffer.SetCombinedImageDescriptor(
				localDescriptor, s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex,
				freeGlobalDescIndex
			);
	}
	else
		for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
			descriptorBuffer.SetCombinedImageDescriptor(
				*textureView, *defaultSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex, freeGlobalDescIndex
			);

	return freeGlobalDescIndex;
}

void RenderEngine::RemoveTexture(size_t index)
{
	// Could be either an only texture descriptor or multiple combined ones. Unbind all.
	const auto u32Index = static_cast<std::uint32_t>(index);

	m_textureManager.RemoveLocalDescriptor<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(u32Index);
	m_textureManager.RemoveCombinedLocalDescriptorTexture(u32Index);

	const std::uint32_t globalDescriptorIndex = m_textureStorage.GetTextureBindingIndex(index);
	m_textureManager.SetAvailableIndex<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(
		globalDescriptorIndex, true
	);
	m_textureManager.SetAvailableIndex<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(
		globalDescriptorIndex, true
	);

	m_textureStorage.RemoveTexture(index);
}

void RenderEngine::SetCommonGraphicsDescriptorBufferLayout(
	VkShaderStageFlags cameraShaderStage
) noexcept {
	m_cameraManager.SetDescriptorBufferLayoutGraphics(
		m_graphicsDescriptorBuffers, GetCameraBindingSlot(), s_vertexShaderSetLayoutIndex,
		cameraShaderStage
	);
}

void RenderEngine::UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails)
{
	m_externalResourceManager.UpdateDescriptor(m_graphicsDescriptorBuffers, bindingDetails);
}

void RenderEngine::UploadExternalBufferGPUOnlyData(
	std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
	size_t dstBufferOffset
) {
	m_externalResourceManager.UploadExternalBufferGPUOnlyData(
		m_stagingManager, m_temporaryDataBuffer,
		externalBufferIndex, std::move(cpuData), srcDataSizeInBytes, dstBufferOffset
	);
}

void RenderEngine::QueueExternalBufferGPUCopy(
	std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
	size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
) {
	m_externalResourceManager.QueueExternalBufferGPUCopy(
		externalBufferSrcIndex, externalBufferDstIndex, dstBufferOffset, srcBufferOffset,
		srcDataSizeInBytes, m_temporaryDataBuffer
	);

	m_copyNecessary = true;
}
