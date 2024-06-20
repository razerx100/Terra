#include <ViewportAndScissorManager.hpp>

ViewportAndScissorManager::ViewportAndScissorManager()
	: m_viewport {
		.x        = 0.f,
		.minDepth = 0.f,
		.maxDepth = 1.f
	},
	m_scissor{ .offset = VkOffset2D{ .x = 0, .y = 0 } }
{}

void ViewportAndScissorManager::Resize(std::uint32_t width, std::uint32_t height) noexcept
{
	ResizeViewport(width, height);
	ResizeScissor(width, height);
}

void ViewportAndScissorManager::ResizeViewport(std::uint32_t width, std::uint32_t height) noexcept
{
	m_viewport.width  = static_cast<float>(width);
	m_viewport.height = -1.f * height;
	m_viewport.y      = static_cast<float>(height);
}

void ViewportAndScissorManager::ResizeScissor(std::uint32_t width, std::uint32_t height) noexcept
{
	m_scissor.extent = VkExtent2D{ .width = width, .height = height };
}
