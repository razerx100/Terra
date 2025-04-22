#ifndef VK_VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#define VK_VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <VkCommandQueue.hpp>

class ViewportAndScissorManager
{
public:
	ViewportAndScissorManager();

	void Resize(std::uint32_t width, std::uint32_t height) noexcept;

	void BindViewportAndScissor(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

private:
	void ResizeViewport(std::uint32_t width, std::uint32_t height) noexcept;
	void ResizeScissor(std::uint32_t width, std::uint32_t height) noexcept;

private:
	VkViewport m_viewport;
	VkRect2D   m_scissor;

public:
	ViewportAndScissorManager(const ViewportAndScissorManager& other) noexcept
		: m_viewport{ other.m_viewport }, m_scissor{ other.m_scissor }
	{}
	ViewportAndScissorManager& operator=(const ViewportAndScissorManager& other) noexcept
	{
		m_viewport = other.m_viewport;
		m_scissor  = other.m_scissor;

		return *this;
	}

	ViewportAndScissorManager(ViewportAndScissorManager&& other) noexcept
		: m_viewport{ other.m_viewport }, m_scissor{ other.m_scissor }
	{}
	ViewportAndScissorManager& operator=(ViewportAndScissorManager&& other) noexcept
	{
		m_viewport = other.m_viewport;
		m_scissor  = other.m_scissor;

		return *this;
	}
};
#endif
