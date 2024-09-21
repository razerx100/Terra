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
	m_transferQueue {
		logicalDevice,
		queueFamilyManager->GetQueue(QueueType::TransferQueue),
		queueFamilyManager->GetIndex(QueueType::TransferQueue)
	}, m_transferWait{},
	m_stagingManager{ logicalDevice, &m_memoryManager, m_threadPool.get(), queueFamilyManager },
	m_graphicsDescriptorBuffers{},
	m_textureStorage{ logicalDevice, &m_memoryManager },
	m_textureManager{ logicalDevice, &m_memoryManager },
	m_materialBuffers{ logicalDevice, &m_memoryManager },
	m_cameraManager{ logicalDevice, &m_memoryManager },
	m_depthBuffer{ logicalDevice, &m_memoryManager }, m_renderPass{ logicalDevice },
	m_backgroundColour{ {0.0001f, 0.0001f, 0.0001f, 0.0001f } }, m_viewportAndScissors{},
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

size_t RenderEngine::AddMaterial(std::shared_ptr<Material> material)
{
	const size_t index = m_materialBuffers.Add(std::move(material));

	m_materialBuffers.Update(index);

	m_materialBuffers.SetDescriptorBuffer(
		m_graphicsDescriptorBuffers, s_materialBindingSlot, s_fragmentShaderSetLayoutIndex
	);

	m_copyNecessary = true;

	return index;
}

std::vector<size_t> RenderEngine::AddMaterials(std::vector<std::shared_ptr<Material>>&& materials)
{
	std::vector<size_t> indices = m_materialBuffers.AddMultiple(std::move(materials));

	m_materialBuffers.Update(indices);

	m_materialBuffers.SetDescriptorBuffer(
		m_graphicsDescriptorBuffers, s_materialBindingSlot, s_fragmentShaderSetLayoutIndex
	);

	m_copyNecessary = true;

	return indices;
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
	// have 65535 bound textures at once. There could be more textures. Adding new descriptors
	// would change the PipelineLayout and that would require every single Pipeline Object to be
	// recreated. Which isn't worth it. And it should be fine to have only 65535 textures bound
	// at once.
	assert(
		oFreeGlobalDescIndex.has_value()
		&& "No descriptor available to bind the texture. Free some already bound textures."
	);

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

		for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
			descriptorBuffer.SetCombinedImageDescriptor(
				localDescriptor, s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex,
				freeGlobalDescIndex
			);
	}
	else
		for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
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

void RenderEngine::BeginRenderPass(
	const VKCommandBuffer& graphicsCmdBuffer, const VKFramebuffer& frameBuffer,
	VkExtent2D renderArea
) {
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color        = m_backgroundColour;
	clearValues[1].depthStencil = m_depthBuffer.GetClearValues();

	VkCommandBuffer cmdBuffer   = graphicsCmdBuffer.Get();

	m_renderPass.BeginPass(cmdBuffer, frameBuffer.Get(), renderArea, clearValues);
}

void RenderEngine::Update(VkDeviceSize frameIndex) const noexcept
{
	m_cameraManager.Update(frameIndex);
}

void RenderEngine::SetCommonGraphicsDescriptorBufferLayout(
	VkShaderStageFlagBits cameraShaderStage
) noexcept {
	m_cameraManager.SetDescriptorBufferLayoutGraphics(
		m_graphicsDescriptorBuffers, GetCameraBindingSlot(), s_vertexShaderSetLayoutIndex, cameraShaderStage
	);
	m_materialBuffers.SetDescriptorBufferLayout(
		m_graphicsDescriptorBuffers, s_materialBindingSlot, s_fragmentShaderSetLayoutIndex
	);
}

std::uint32_t RenderEngine::AddMeshBundle([[maybe_unused]] std::unique_ptr<MeshBundleVS> meshBundle)
{
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddMeshBundle([[maybe_unused]] std::unique_ptr<MeshBundleMS> meshBundle)
{
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddModelBundle(
	[[maybe_unused]] std::shared_ptr<ModelBundleVS>&& modelBundle,
	[[maybe_unused]] const ShaderName& fragmentShader
) {
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddModelBundle(
	[[maybe_unused]] std::shared_ptr<ModelBundleMS>&& modelBundle,
	[[maybe_unused]] const ShaderName& fragmentShader
) {
	return std::numeric_limits<std::uint32_t>::max();
}
