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
	VkImageView GetView() const noexcept { return m_depthImage.GetView(); }
	[[nodiscard]]
	const Texture& GetTexture() const noexcept { return m_depthImage.GetTexture(); }

	[[nodiscard]]
	static VkFormat GetDepthFormat() noexcept { return DEPTHFORMAT; }

private:
	VkTextureView m_depthImage;

	static constexpr VkFormat DEPTHFORMAT = VK_FORMAT_D32_SFLOAT;

public:
	DepthBuffer(const DepthBuffer&) = delete;
	DepthBuffer& operator=(const DepthBuffer&) = delete;

	DepthBuffer(DepthBuffer&& other) noexcept : m_depthImage{ std::move(other.m_depthImage) } {}
	DepthBuffer& operator=(DepthBuffer&& other) noexcept
	{
		m_depthImage = std::move(other.m_depthImage);

		return *this;
	}
};
#endif
