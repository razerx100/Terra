#ifndef VK_FRAME_BUFFER_HPP_
#define VK_FRAME_BUFFER_HPP_
#include <vulkan/vulkan.hpp>
#include <VKRenderPass.hpp>
#include <utility>

namespace Terra
{
class VKFramebuffer
{
public:
	VKFramebuffer(VkDevice device) : m_device{ device }, m_framebuffer{ VK_NULL_HANDLE } {}
	~VKFramebuffer() noexcept;

	void Create(
		const VKRenderPass& renderPass, std::uint32_t width, std::uint32_t height,
		std::span<VkImageView> attachments
	);

	[[nodiscard]]
	VkFramebuffer Get() const noexcept { return m_framebuffer; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice      m_device;
	VkFramebuffer m_framebuffer;

public:
	VKFramebuffer(const VKFramebuffer&) = delete;
	VKFramebuffer& operator=(const VKFramebuffer&) = delete;

	VKFramebuffer(VKFramebuffer&& other) noexcept
		: m_device{ other.m_device },
		m_framebuffer{ std::exchange(other.m_framebuffer, VK_NULL_HANDLE) }
	{}
	VKFramebuffer& operator=(VKFramebuffer&& other) noexcept
	{
		SelfDestruct();

		m_device      = other.m_device;
		m_framebuffer = std::exchange(other.m_framebuffer, VK_NULL_HANDLE);

		return *this;
	}
};
}
#endif
