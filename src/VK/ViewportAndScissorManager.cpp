#include <ViewportAndScissorManager.hpp>

ViewportAndScissorManager::ViewportAndScissorManager(const Args& arguments)
	: m_viewport{}, m_scissor{} {
	m_viewport.x = 0.0f;
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	std::uint32_t width = arguments.width.value();
	std::uint32_t height = arguments.height.value();

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
	m_viewport.height = -1.f * static_cast<float>(height);
	m_viewport.y = static_cast<float>(height);
}

void ViewportAndScissorManager::ResizeScissor(
	std::uint32_t width, std::uint32_t height
) noexcept {
	m_scissor.extent.width = width;
	m_scissor.extent.height = height;
}
