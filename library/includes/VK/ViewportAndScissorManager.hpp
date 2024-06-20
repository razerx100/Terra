#ifndef VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#define VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <cstdint>

class ViewportAndScissorManager
{
public:
	ViewportAndScissorManager();

	[[nodiscard]]
	VkViewport GetViewport() const noexcept { return m_viewport; }
	[[nodiscard]]
	VkRect2D GetScissor() const noexcept { return m_scissor; }
	[[nodiscard]]
	const VkViewport* GetViewportRef() const noexcept { return &m_viewport; }
	[[nodiscard]]
	const VkRect2D* GetScissorRef() const noexcept { return &m_scissor; }

	void Resize(std::uint32_t width, std::uint32_t height) noexcept;

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
