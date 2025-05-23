#ifndef VK_EXTERNAL_BUFFER_HPP_
#define VK_EXTERNAL_BUFFER_HPP_
#include <ExternalBuffer.hpp>
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkResourceBarriers2.hpp>

namespace Terra
{
class VkExternalBuffer
{
public:
	VkExternalBuffer(
		VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType,
		VkBufferUsageFlags usageFlags
	);

	void Create(size_t bufferSize);

	void Destroy() noexcept { m_buffer.Destroy(); }

	[[nodiscard]]
	size_t BufferSize() const noexcept { return m_buffer.BufferSize(); }

	[[nodiscard]]
	std::uint8_t* CPUHandle() const { return m_buffer.CPUHandle(); }

	[[nodiscard]]
	const Buffer& GetBuffer() const noexcept { return m_buffer; }

private:
	Buffer             m_buffer;
	VkBufferUsageFlags m_usageFlags;

public:
	VkExternalBuffer(const VkExternalBuffer&) = delete;
	VkExternalBuffer& operator=(const VkExternalBuffer&) = delete;

	VkExternalBuffer(VkExternalBuffer&& other) noexcept
		: m_buffer{ std::move(other.m_buffer) }, m_usageFlags{ other.m_usageFlags }
	{}
	VkExternalBuffer& operator=(VkExternalBuffer&& other) noexcept
	{
		m_buffer     = std::move(other.m_buffer);
		m_usageFlags = other.m_usageFlags;

		return *this;
	}
};

class VkExternalTexture
{
public:
	VkExternalTexture(VkDevice device, MemoryManager* memoryManager);

	void Create(
		std::uint32_t width, std::uint32_t height, ExternalFormat format,
		ExternalTexture2DType type, const ExternalTextureCreationFlags& creationFlags = {}
	);

	void Destroy() noexcept;

	[[nodiscard]]
	RendererType::Extent GetExtent() const noexcept
	{
		const VkExtent3D vkExtent = m_textureView.GetTexture().GetExtent();

		return RendererType::Extent{ .width = vkExtent.width, .height = vkExtent.height };
	}

	void SetCurrentPipelineStage(VkPipelineStageFlagBits newStage) noexcept
	{
		m_currentPipelineStage = newStage;
	}

	[[nodiscard]]
	VkPipelineStageFlagBits GetCurrentPipelineStage() const noexcept
	{
		return m_currentPipelineStage;
	}

	// This actually won't change the state. Need to use the barrier builder and then execute it
	// on a CommandQueue. Need to get the barrier builder through this function, so we can remember
	// the state of the texture.
	[[nodiscard]]
	ImageBarrierBuilder TransitionState(
		VkAccessFlagBits newAccess, VkImageLayout newLayout, VkPipelineStageFlagBits newStage
	) noexcept;

	[[nodiscard]]
	const VkTextureView& GetTextureView() const noexcept { return m_textureView; }

private:
	VkTextureView           m_textureView;
	// For now I don't need to use Access2, so using Access 1 to save memory.
	VkAccessFlagBits        m_currentAccessState;
	VkImageLayout           m_currentLayoutState;
	VkPipelineStageFlagBits m_currentPipelineStage;

public:
	VkExternalTexture(const VkExternalTexture&) = delete;
	VkExternalTexture& operator=(const VkExternalTexture&) = delete;

	VkExternalTexture(VkExternalTexture&& other) noexcept
		: m_textureView{ std::move(other.m_textureView) },
		m_currentAccessState{ other.m_currentAccessState },
		m_currentLayoutState{ other.m_currentLayoutState },
		m_currentPipelineStage{ other.m_currentPipelineStage }
	{}
	VkExternalTexture& operator=(VkExternalTexture&& other) noexcept
	{
		m_textureView          = std::move(other.m_textureView);
		m_currentAccessState   = other.m_currentAccessState;
		m_currentLayoutState   = other.m_currentLayoutState;
		m_currentPipelineStage = other.m_currentPipelineStage;

		return *this;
	}
};
}
#endif
