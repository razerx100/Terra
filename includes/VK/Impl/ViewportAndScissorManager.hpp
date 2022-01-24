#ifndef __VIEWPORT_AND_SCISSOR_MANAGER_HPP__
#define __VIEWPORT_AND_SCISSOR_MANAGER_HPP__
#include <IViewportAndScissorManager.hpp>

class ViewportAndScissorManager : public IViewportAndScissorManager {
public:
	ViewportAndScissorManager(std::uint32_t width, std::uint32_t height);

	const VkViewport* GetViewportRef() const noexcept override;
	const VkRect2D* GetScissorRef() const noexcept override;

	void Resize(std::uint32_t width, std::uint32_t height) noexcept override;

private:
	void ResizeViewport(std::uint32_t width, std::uint32_t height) noexcept;
	void ResizeScissor(std::uint32_t width, std::uint32_t height) noexcept;

private:
	VkViewport m_viewport;
	VkRect2D m_scissor;
};
#endif
