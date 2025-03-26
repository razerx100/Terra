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
	m_memoryManager{ std::make_unique<MemoryManager>(physicalDevice, logicalDevice, 20_MB, 400_KB) },
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
	m_stagingManager{ logicalDevice, m_memoryManager.get(), m_threadPool.get(), queueFamilyManager},
	m_externalResourceManager{
		std::make_unique<VkExternalResourceManager>(logicalDevice, m_memoryManager.get())
	},
	m_graphicsDescriptorBuffers{},
	m_graphicsPipelineLayout{ logicalDevice },
	m_textureStorage{ logicalDevice, m_memoryManager.get() },
	m_textureManager{ logicalDevice, m_memoryManager.get() },
	m_cameraManager{ logicalDevice, m_memoryManager.get() },
	m_viewportAndScissors{}, m_temporaryDataBuffer{}, m_copyNecessary{ false }
{
	VkDescriptorBuffer::SetDescriptorBufferInfo(physicalDevice);

	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_graphicsDescriptorBuffers.emplace_back(
			logicalDevice, m_memoryManager.get(), s_graphicsPipelineSetLayoutCount
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

size_t RenderEngine::AddTextureAsCombined(STexture&& texture)
{
	const size_t textureIndex = m_textureStorage.AddTexture(
		std::move(texture), m_stagingManager, m_temporaryDataBuffer
	);

	m_copyNecessary = true;

	return textureIndex;
}

void RenderEngine::UnbindCombinedTexture(
	size_t textureIndex, std::uint32_t bindingindex, size_t samplerIndex
) {
	// This function shouldn't need to wait for the GPU to finish, as it isn't doing
	// anything on the GPU side.
	static constexpr VkDescriptorType DescType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	assert(
		!std::empty(m_graphicsDescriptorBuffers)
		&& "The Descriptor Buffers should be created before calling this."
	);

	void const* globalDescriptor = m_graphicsDescriptorBuffers.front().GetDescriptor<DescType>(
		s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex, bindingindex
	);

	const std::uint32_t localCacheIndex = m_textureManager.GetFirstFreeLocalDescriptor<DescType>();

	m_textureManager.SetLocalDescriptor<DescType>(globalDescriptor, localCacheIndex);

	m_textureManager.SetLocalDescriptorAvailability<DescType>(localCacheIndex, false);

	m_textureManager.SetBindingAvailability<DescType>(bindingindex, true);

	m_textureStorage.SetCombinedCacheDetails(
		static_cast<std::uint32_t>(textureIndex),
		static_cast<std::uint32_t>(samplerIndex),
		localCacheIndex
	);
}

void RenderEngine::UnbindCombinedTexture(size_t textureIndex, std::uint32_t bindingindex)
{
	UnbindCombinedTexture(textureIndex, bindingindex, m_textureStorage.GetDefaultSamplerIndex());
}

std::uint32_t RenderEngine::BindCombinedTextureCommon(
	VkTextureView const* textureView, VKSampler const* sampler,
	std::optional<std::uint32_t> oLocalCacheIndex
) {
	static constexpr VkDescriptorType DescType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	static constexpr TextureDescType TexDescType = TextureDescType::CombinedTexture;

	std::optional<size_t> oFreeGlobalDescIndex = m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

	// If there is no free global index, increase the limit. Right now it should be possible to
	// have 65535 bound textures at once. There could be more textures.
	if (!oFreeGlobalDescIndex)
	{
		m_textureManager.IncreaseMaximumBindingCount<TexDescType>();

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

	const auto freeGlobalDescIndex = static_cast<std::uint32_t>(oFreeGlobalDescIndex.value());

	m_textureManager.SetBindingAvailability<DescType>(freeGlobalDescIndex, false);

	if (oLocalCacheIndex)
	{
		const std::uint32_t localCacheIndex = oLocalCacheIndex.value();

		void const* localDescriptor = m_textureManager.GetLocalDescriptor<DescType>(
			localCacheIndex
		);

		m_textureManager.SetLocalDescriptorAvailability<DescType>(localCacheIndex, true);

		for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
			descriptorBuffer.SetCombinedImageDescriptor(
				localDescriptor, s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex,
				freeGlobalDescIndex
			);
	}
	else
		for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
			descriptorBuffer.SetCombinedImageDescriptor(
				*textureView, *sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex, freeGlobalDescIndex
			);

	return freeGlobalDescIndex;
}

std::uint32_t RenderEngine::BindCombinedTexture(size_t textureIndex, size_t samplerIndex)
{
	VkTextureView const* textureView = m_textureStorage.GetPtr(textureIndex);
	VKSampler const* sampler         = m_textureStorage.GetSamplerPtr(samplerIndex);

	// The current caching system only works for read only single textures which are bound to
	// multiple descriptor buffers. Because we only cache one of them.
	std::optional<std::uint32_t> localCacheIndex = m_textureStorage.GetAndRemoveCombinedLocalDescIndex(
		static_cast<std::uint32_t>(textureIndex), static_cast<std::uint32_t>(samplerIndex)
	);

	return BindCombinedTextureCommon(textureView, sampler, localCacheIndex);
}

std::uint32_t RenderEngine::BindCombinedTexture(size_t index)
{
	return BindCombinedTexture(index, m_textureStorage.GetDefaultSamplerIndex());
}

void RenderEngine::UnbindExternalTexture(std::uint32_t bindingindex)
{
	static constexpr VkDescriptorType DescType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	m_textureManager.SetBindingAvailability<DescType>(bindingindex, true);
}

void RenderEngine::RebindExternalTexture(size_t textureIndex, std::uint32_t bindingIndex)
{
	RebindExternalTexture(textureIndex, m_textureStorage.GetDefaultSamplerIndex(), bindingIndex);
}

void RenderEngine::RebindExternalTexture(
	size_t textureIndex, size_t samplerIndex, std::uint32_t bindingIndex
) {
	VkExternalResourceFactory* resourceFactory = m_externalResourceManager->GetVkResourceFactory();

	VkTextureView const* textureView = &resourceFactory->GetVkTextureView(textureIndex);

	VKSampler const* sampler         = m_textureStorage.GetSamplerPtr(samplerIndex);

	for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.SetCombinedImageDescriptor(
			*textureView, *sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex, bindingIndex
		);
}

std::uint32_t RenderEngine::BindExternalTexture(size_t textureIndex)
{
	return BindExternalTexture(textureIndex, m_textureStorage.GetDefaultSamplerIndex());
}

std::uint32_t RenderEngine::BindExternalTexture(size_t textureIndex, size_t samplerIndex)
{
	VkExternalResourceFactory* resourceFactory = m_externalResourceManager->GetVkResourceFactory();

	VkTextureView const* textureView = &resourceFactory->GetVkTextureView(textureIndex);

	VKSampler const* sampler         = m_textureStorage.GetSamplerPtr(samplerIndex);

	// Can't cache as the underlying resource might change or we might have a separate texture
	// on each descriptor buffer.
	return BindCombinedTextureCommon(textureView, sampler, {});
}

void RenderEngine::RemoveTexture(size_t textureIndex)
{
	// Could be either an only texture descriptor or multiple combined ones. Unbind all.
	const auto u32Index = static_cast<std::uint32_t>(textureIndex);

	std::vector<std::uint32_t> localCombinedCacheIndices
		= m_textureStorage.GetAndRemoveCombinedCacheDetailsTexture(u32Index);

	for (std::uint32_t localCacheIndex : localCombinedCacheIndices)
		m_textureManager.SetLocalDescriptorAvailability<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(
			localCacheIndex, true
		);

	m_textureStorage.RemoveTexture(textureIndex);
}

std::vector<std::uint32_t> RenderEngine::AddModelsToBuffer(
	const ModelBundle& modelBundle, ModelBuffers& modelBuffers
) noexcept {
	std::vector<std::shared_ptr<Model>> models = modelBundle.GetModels();

	return modelBuffers.AddMultipleRU32(std::move(models));
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
	m_externalResourceManager->UpdateDescriptor(m_graphicsDescriptorBuffers, bindingDetails);
}

void RenderEngine::UploadExternalBufferGPUOnlyData(
	std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
	size_t dstBufferOffset
) {
	m_externalResourceManager->UploadExternalBufferGPUOnlyData(
		m_stagingManager, m_temporaryDataBuffer,
		externalBufferIndex, std::move(cpuData), srcDataSizeInBytes, dstBufferOffset
	);
}

void RenderEngine::QueueExternalBufferGPUCopy(
	std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
	size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
) {
	m_externalResourceManager->QueueExternalBufferGPUCopy(
		externalBufferSrcIndex, externalBufferDstIndex, dstBufferOffset, srcBufferOffset,
		srcDataSizeInBytes, m_temporaryDataBuffer
	);

	m_copyNecessary = true;
}
