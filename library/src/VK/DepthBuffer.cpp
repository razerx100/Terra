#include <DepthBuffer.hpp>

DepthBuffer::DepthBuffer(VkDevice device, MemoryManager* memoryManager)
	: m_depthImage{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT } {}

void DepthBuffer::Create(std::uint32_t width, std::uint32_t height)
{
	m_depthImage.CreateView(
		width, height, GetDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, {}
	);
}
