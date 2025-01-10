#ifndef DEPTH_BUFFER_HPP_
#define DEPTH_BUFFER_HPP_
#include <vulkan/vulkan.hpp>
#include <VkTextureView.hpp>

class DepthBuffer
{
public:
	DepthBuffer(VkDevice device, MemoryManager* memoryManager);

	void Create(std::uint32_t width, std::uint32_t height);

	[[nodiscard]]
	VkImageView GetVkView() const noexcept { return m_depthImage.GetVkView(); }
	[[nodiscard]]
	const VKImageView& GetView() const noexcept { return m_depthImage.GetView(); }
	[[nodiscard]]
	const Texture& GetTexture() const noexcept { return m_depthImage.GetTexture(); }

	[[nodiscard]]
	VkFormat GetFormat() noexcept { return m_depthImage.GetTexture().Format(); }
	[[nodiscard]]
	VkClearDepthStencilValue GetClearValues() const noexcept { return m_depthStencilValue; }

private:
	VkTextureView            m_depthImage;
	VkClearDepthStencilValue m_depthStencilValue;

public:
	DepthBuffer(const DepthBuffer&) = delete;
	DepthBuffer& operator=(const DepthBuffer&) = delete;

	DepthBuffer(DepthBuffer&& other) noexcept
		: m_depthImage{ std::move(other.m_depthImage) }, m_depthStencilValue{ other.m_depthStencilValue }
	{}
	DepthBuffer& operator=(DepthBuffer&& other) noexcept
	{
		m_depthImage        = std::move(other.m_depthImage);
		m_depthStencilValue = other.m_depthStencilValue;

		return *this;
	}
};
#endif
