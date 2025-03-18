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

	bindingIndices[index] = bindingIndex;
}

size_t TextureStorage::AddTexture(
	STexture&& texture, StagingBufferManager& stagingBufferManager, TemporaryDataBufferGPU& tempBuffer
) {
	const size_t index = m_textures.Add(
		VkTextureView{ m_device, m_memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT }
	);

	VkTextureView* textureViewPtr = &m_textures[index];

	textureViewPtr->CreateView2D(
		texture.width, texture.height, s_textureFormat,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, {}
	);

	stagingBufferManager.AddTextureView(
		std::move(texture.data), textureViewPtr, {}, QueueType::GraphicsQueue,
		VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, tempBuffer
	);

	// Should be fine because of the deque.
	m_transitionQueue.push(textureViewPtr);

	return index;
}

size_t TextureStorage::AddSampler(const VkSamplerCreateInfoBuilder& builder)
{
	VKSampler sampler{ m_device };

	sampler.Create(builder);

	return m_samplers.Add(std::move(sampler));
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
	m_textures[index].Destroy();
	m_textures.RemoveElement(index);
}

void TextureStorage::RemoveSampler(size_t index)
{
	if (index != s_defaultSamplerIndex)
		m_samplers.RemoveElement(index);

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

	if (combinedDescCount)
		descriptorBuffer.AddBinding(
			combinedTexturesBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			combinedDescCount, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
		);

	const auto sampledDescCount = static_cast<std::uint32_t>(
		std::size(m_availableIndicesSampledTextures)
	);

	if (sampledDescCount)
		descriptorBuffer.AddBinding(
			sampledTexturesBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			sampledDescCount, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
		);

	const auto samplerDescCount = static_cast<std::uint32_t>(
		std::size(m_availableIndicesSamplers)
	);

	if (samplerDescCount)
		descriptorBuffer.AddBinding(
			samplersBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_SAMPLER,
			samplerDescCount, VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
		);
}
