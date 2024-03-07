#include <DepthBuffer.hpp>

DepthBuffer::DepthBuffer(VkDevice device, MemoryManager* memoryManager)
	: m_depthImage{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_depthStencilValue{ 0.f, 0u }
{}

void DepthBuffer::Create(std::uint32_t width, std::uint32_t height)
{
	m_depthImage.CreateView2D(
		width, height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, {}
	);

	m_depthStencilValue.depth = 1.f;
}
