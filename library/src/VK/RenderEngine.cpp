#include <RenderEngine.hpp>

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
	}, m_transferQueue{
		logicalDevice,
		queueFamilyManager->GetQueue(QueueType::TransferQueue),
		queueFamilyManager->GetIndex(QueueType::TransferQueue)
	},
	m_stagingManager{
		logicalDevice, &m_memoryManager, &m_transferQueue, m_threadPool.get(), queueFamilyManager
	}, m_graphicsDescriptorBuffers{},
	m_textureStorage{ logicalDevice, &m_memoryManager },
	m_textureManager{ logicalDevice, &m_memoryManager },
	m_materialBuffers{ logicalDevice, &m_memoryManager },
	m_cameraManager{ logicalDevice, &m_memoryManager },
	m_depthBuffer{ logicalDevice, &m_memoryManager }, m_renderPass{ logicalDevice },
	m_backgroundColour{ {0.0001f, 0.0001f, 0.0001f, 0.0001f } }, m_viewportAndScissors{}
{
	for (size_t _ = 0u; _ < frameCount; ++_)
		m_graphicsDescriptorBuffers.emplace_back(logicalDevice, &m_memoryManager);

	m_graphicsQueue.CreateCommandBuffers(static_cast<std::uint32_t>(frameCount));
	m_transferQueue.CreateCommandBuffers(1u);
}

size_t RenderEngine::AddMaterial(std::shared_ptr<Material> material)
{
	const size_t index = m_materialBuffers.Add(std::move(material));

	m_materialBuffers.Update(index);

	return index;
}

std::vector<size_t> RenderEngine::AddMaterials(std::vector<std::shared_ptr<Material>>&& materials)
{
	std::vector<size_t> indices = m_materialBuffers.AddMultiple(std::move(materials));

	m_materialBuffers.Update(indices);

	return indices;
}

void RenderEngine::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept
{
	m_backgroundColour = {
		{ colourVector.at(0u), colourVector.at(1), colourVector.at(2), colourVector.at(3) }
	};
}

size_t RenderEngine::AddTextureAsCombined(
	std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
) {
	return AddTextureAsCombined(
		std::move(textureData), width, height, m_textureStorage.GetDefaultSamplerIndex()
	);
}

size_t RenderEngine::AddTextureAsCombined(
	std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height,
	size_t samplerIndex
) {
	const size_t textureIndex = m_textureStorage.AddTexture(
		std::move(textureData), width, height, m_stagingManager
	);

	BindCombinedTexture(textureIndex, samplerIndex);

	return textureIndex;
}

void RenderEngine::UnbindCombinedTexture(size_t index)
{
	UnbindCombinedTexture(index, m_textureStorage.GetDefaultSamplerIndex());
}

void RenderEngine::UnbindCombinedTexture(size_t textureIndex, size_t samplerIndex)
{
	static constexpr VkDescriptorType DescType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	const std::uint32_t globalDescIndex = m_textureStorage.GetTextureBindingIndex(textureIndex);

	if (!std::empty(m_graphicsDescriptorBuffers))
	{
		void const* globalDescriptor = m_graphicsDescriptorBuffers.front().GetDescriptor<DescType>(
			GetCombinedTextureBindingSlot(), globalDescIndex
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

void RenderEngine::BindCombinedTexture(size_t index)
{
	BindCombinedTexture(index, m_textureStorage.GetDefaultSamplerIndex());
}

void RenderEngine::BindCombinedTexture(size_t textureIndex, size_t samplerIndex)
{
	static constexpr VkDescriptorType DescType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkTextureView const* textureView = m_textureStorage.GetPtr(textureIndex);
	VKSampler const* defaultSampler  = m_textureStorage.GetSamplerPtr(samplerIndex);

	auto oFreeGlobalDescIndex        = m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

	// If there is no free global index, increase it and recreate the global descriptor buffers.
	if (!oFreeGlobalDescIndex)
	{
		m_textureManager.IncreaseAvailableIndices(TextureDescType::CombinedTexture);

		for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		{
			m_textureManager.SetDescriptorBufferLayout(
				descriptorBuffer, GetCombinedTextureBindingSlot(), GetSampledTextureBindingSlot(),
				GetSamplerBindingSlot()
			);

			descriptorBuffer.RecreateBuffer();
		}

		oFreeGlobalDescIndex = m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

		// There should be free indices now. So, just gonna do an assert.
		assert(oFreeGlobalDescIndex.has_value() && "No free global desc even after recreation");
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

		for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
			descriptorBuffer.SetCombinedImageDescriptor(
				localDescriptor, GetCombinedTextureBindingSlot(), freeGlobalDescIndex
			);
	}
	else
		for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
			descriptorBuffer.SetCombinedImageDescriptor(
				*textureView, *defaultSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				GetCombinedTextureBindingSlot(), freeGlobalDescIndex
			);
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

void RenderEngine::Resize(
	std::uint32_t width, std::uint32_t height,
	bool hasSwapchainFormatChanged, VkFormat swapchainFormat
) {
	m_depthBuffer.Create(width, height);

	if (hasSwapchainFormatChanged)
		m_renderPass.Create(
			RenderPassBuilder{}
			.AddColourAttachment(swapchainFormat)
			.AddDepthAttachment(m_depthBuffer.GetFormat())
			.Build()
		);

	m_viewportAndScissors.Resize(width, height);
}

void RenderEngine::BeginRenderPass(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea
) {
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color        = m_backgroundColour;
	clearValues[1].depthStencil = m_depthBuffer.GetClearValues();

	VkCommandBuffer cmdBuffer   = m_graphicsQueue.GetCommandBuffer(frameIndex).Get();

	m_renderPass.BeginPass(cmdBuffer, frameBuffer.Get(), renderArea, clearValues);
}

std::uint32_t RenderEngine::AddMeshBundle([[maybe_unused]] std::unique_ptr<MeshBundleVS> meshBundle)
{
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddMeshBundle([[maybe_unused]] std::unique_ptr<MeshBundleMS> meshBundle)
{
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddModel(
	[[maybe_unused]] std::shared_ptr<ModelVS>&& model,
	[[maybe_unused]] const std::wstring& fragmentShader
) {
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddModelBundle(
	[[maybe_unused]] std::vector<std::shared_ptr<ModelVS>>&& modelBundle,
	[[maybe_unused]] const std::wstring& fragmentShader
) {
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddModel(
	[[maybe_unused]] std::shared_ptr<ModelMS>&& model,
	[[maybe_unused]] const std::wstring& fragmentShader
) {
	return std::numeric_limits<std::uint32_t>::max();
}

std::uint32_t RenderEngine::AddModelBundle(
	[[maybe_unused]] std::vector<std::shared_ptr<ModelMS>>&& modelBundle,
	[[maybe_unused]] const std::wstring& fragmentShader
) {
	return std::numeric_limits<std::uint32_t>::max();
}
