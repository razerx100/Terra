#include <VkViewportAndScissorManager.hpp>

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

void ViewportAndScissorManager::BindViewportAndScissor(
	const VKCommandBuffer& graphicsCmdBuffer
) const noexcept {
	VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

	vkCmdSetViewport(cmdBuffer, 0u, 1u, &m_viewport);
	vkCmdSetScissor(cmdBuffer, 0u, 1u, &m_scissor);
}
