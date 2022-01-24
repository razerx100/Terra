#include <ViewportAndScissorManager.hpp>

ViewportAndScissorManager::ViewportAndScissorManager(
	std::uint32_t width, std::uint32_t height
) : m_viewport{}, m_scissor{} {
	m_viewport.x = 0.0f;
	m_viewport.y = 0.0f;
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
	ResizeViewport(width, height);

	m_scissor.offset = { 0, 0 };
	ResizeScissor(width, height);
}

const VkViewport* ViewportAndScissorManager::GetViewportRef() const noexcept {
	return &m_viewport;
}

const VkRect2D* ViewportAndScissorManager::GetScissorRef() const noexcept {
	return &m_scissor;
}

void ViewportAndScissorManager::Resize(std::uint32_t width, std::uint32_t height) noexcept {
	ResizeViewport(width, height);
	ResizeScissor(width, height);
}

void ViewportAndScissorManager::ResizeViewport(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_viewport.width = static_cast<float>(width);
	m_viewport.height = static_cast<float>(height);
}

void ViewportAndScissorManager::ResizeScissor(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_scissor.extent.width = width;
	m_scissor.extent.height = height;
}
