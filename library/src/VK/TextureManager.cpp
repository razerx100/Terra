#include <TextureManager.hpp>
#include <VkResourceBarriers2.hpp>
#include <ranges>
#include <algorithm>

// Texture storage
void TextureStorage::SetBindingIndex(size_t textureIndex, std::uint32_t bindingIndex) noexcept
{
	if (std::size(m_textureBindingIndices) <= textureIndex)
	{
		m_textureBindingIndices.resize(textureIndex + 1u);
	}

	m_textureBindingIndices.at(textureIndex) = bindingIndex;
}

size_t TextureStorage::AddTexture(
	std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height,
	StagingBufferManager& stagingBufferManager
) {
	VkTextureView textureView{ m_device, m_memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

	auto index = std::numeric_limits<size_t>::max();

	VkTextureView* textureViewPtr = nullptr;

	{
		auto result = std::ranges::find(m_availableIndices, true);

		if (result != std::end(m_availableIndices))
		{
			index          = static_cast<size_t>(std::distance(std::begin(m_availableIndices), result));
			textureViewPtr = &m_textures.at(index);
		}
		else
		{
			index                      = std::size(m_textures);
			VkTextureView& returnedRef = m_textures.emplace_back(std::move(textureView));
			m_availableIndices.emplace_back(false);
			textureViewPtr             = &returnedRef;
		}
	}

	textureViewPtr->CreateView2D(
		static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height),
		s_textureFormat,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, {}
	);

	stagingBufferManager.AddTextureView(
		textureData.get(), textureViewPtr->GetTexture().Size(), textureViewPtr, {},
		QueueType::GraphicsQueue, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
	);

	m_transitionQueue.push(textureViewPtr);

	return index;
}

void TextureStorage::CleanupTempData() noexcept
{
	m_textureData = std::vector<std::unique_ptr<std::uint8_t>>{};
}

void TextureStorage::TransitionQueuedTextures(VKCommandBuffer& graphicsCmdBuffer)
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
	m_availableIndices.at(index) = true;
	m_textures.at(index).Destroy();
}
