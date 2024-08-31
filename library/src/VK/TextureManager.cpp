#include <TextureManager.hpp>
#include <VkResourceBarriers2.hpp>

// Texture storage
void TextureStorage::SetBindingIndex(
	size_t index, std::uint32_t bindingIndex, std::vector<std::uint32_t>& bindingIndices
) noexcept {
	if (std::size(bindingIndices) <= index)
	{
		bindingIndices.resize(index + 1u);
	}

	bindingIndices.at(index) = bindingIndex;
}

size_t TextureStorage::AddTexture(
	STexture&& texture, StagingBufferManager& stagingBufferManager, TemporaryDataBufferGPU& tempBuffer
) {
	VkTextureView textureView{ m_device, m_memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

	auto index = std::numeric_limits<size_t>::max();

	VkTextureView* textureViewPtr = nullptr;

	{
		auto result = std::ranges::find(m_availableTextureIndices, true);

		if (result != std::end(m_availableTextureIndices))
		{
			index          = static_cast<size_t>(
				std::distance(std::begin(m_availableTextureIndices), result)
			);
			textureViewPtr = &m_textures.at(index);
		}
		else
		{
			index                      = std::size(m_textures);
			VkTextureView& returnedRef = m_textures.emplace_back(std::move(textureView));
			m_availableTextureIndices.emplace_back(false);
			textureViewPtr             = &returnedRef;
		}
	}

	textureViewPtr->CreateView2D(
		texture.width, texture.height, s_textureFormat,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, {}
	);

	stagingBufferManager.AddTextureView(
		std::move(texture.data), textureViewPtr, {}, QueueType::GraphicsQueue,
		VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, tempBuffer
	);

	m_transitionQueue.push(textureViewPtr);

	return index;
}

size_t TextureStorage::AddSampler(const VkSamplerCreateInfoBuilder& builder)
{
	VKSampler sampler{ m_device };

	auto index = std::numeric_limits<size_t>::max();

	VKSampler* samplerPtr = nullptr;

	{
		auto result = std::ranges::find(m_availableSamplerIndices, true);

		if (result != std::end(m_availableSamplerIndices))
		{
			index      = static_cast<size_t>(
				std::distance(std::begin(m_availableSamplerIndices), result)
			);
			samplerPtr = &m_samplers.at(index);
		}
		else
		{
			index                  = std::size(m_samplers);
			VKSampler& returnedRef = m_samplers.emplace_back(std::move(sampler));
			m_availableSamplerIndices.emplace_back(false);
			samplerPtr             = &returnedRef;
		}
	}

	samplerPtr->Create(builder);

	return index;
}

void TextureStorage::TransitionQueuedTextures(const VKCommandBuffer& graphicsCmdBuffer)
{
	while (!std::empty(m_transitionQueue))
	{
		VkTextureView const* textureViewPtr = m_transitionQueue.front();
		m_transitionQueue.pop();

		VkImageBarrier2{}.AddMemoryBarrier(
			ImageBarrierBuilder{}
			.Image(*textureViewPtr)
			.AccessMasks(VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)
			.Layouts(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			.StageMasks(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT)
		).RecordBarriers(graphicsCmdBuffer.Get());
	}
}

void TextureStorage::RemoveTexture(size_t index)
{
	m_availableTextureIndices.at(index) = true;
	m_textures.at(index).Destroy();
}

void TextureStorage::RemoveSampler(size_t index)
{
	if (index != s_defaultSamplerIndex)
		m_availableSamplerIndices.at(index) = true;
	// Don't need to destroy this like textures, as it doesn't require any buffer allocations.
}

// Texture Manager
void TextureManager::SetDescriptorBufferLayout(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t combinedTexturesBindingSlot,
	std::uint32_t sampledTexturesBindingSlot, std::uint32_t samplersBindingSlot,
	size_t setLayoutIndex
) const noexcept {
	const auto combinedDescCount = static_cast<std::uint32_t>(
		std::size(m_availableIndicesCombinedTextures)
	);

	if(combinedDescCount)
	{
		descriptorBuffer.AddBinding(
			combinedTexturesBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			combinedDescCount, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
		);
		descriptorBuffer.IncreaseDescriptorCount(
			combinedTexturesBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			combinedDescCount - s_combinedTextureDescriptorCount, combinedDescCount
		);
	}

	const auto sampledDescCount = static_cast<std::uint32_t>(
		std::size(m_availableIndicesSampledTextures)
	);

	if (sampledDescCount)
	{
		descriptorBuffer.AddBinding(
			sampledTexturesBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			sampledDescCount, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
		);
		descriptorBuffer.IncreaseDescriptorCount(
			sampledTexturesBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			sampledDescCount - s_sampledTextureDescriptorCount, sampledDescCount
		);
	}

	const auto samplerDescCount = static_cast<std::uint32_t>(
		std::size(m_availableIndicesSamplers)
	);

	if (samplerDescCount)
	{
		descriptorBuffer.AddBinding(
			samplersBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_SAMPLER,
			samplerDescCount, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
		);
		descriptorBuffer.IncreaseDescriptorCount(
			samplersBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_SAMPLER,
			samplerDescCount - s_samplerDescriptorCount, samplerDescCount
		);
	}
}

std::optional<std::uint32_t> TextureManager::FindFreeIndex(
	const std::vector<bool>& availableIndices
) noexcept {
	auto result = std::ranges::find(availableIndices, true);

	if (result != std::end(availableIndices))
		return static_cast<std::uint32_t>(std::distance(std::begin(availableIndices), result));

	return {};
}
