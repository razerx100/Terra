#include <VkFramebuffer.hpp>

namespace Terra
{
// Framebuffer
VKFramebuffer::~VKFramebuffer() noexcept
{
	SelfDestruct();
}

void VKFramebuffer::SelfDestruct() noexcept
{
	vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
}

void VKFramebuffer::Create(
	const VKRenderPass& renderPass, std::uint32_t width, std::uint32_t height,
	std::span<VkImageView> attachments
) {
	VkFramebufferCreateInfo frameBufferInfo
	{
		.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass      = renderPass.Get(),
		.attachmentCount = static_cast<std::uint32_t>(std::size(attachments)),
		.pAttachments    = std::data(attachments),
		.width           = width,
		.height          = height,
		.layers          = 1u
	};

	if (m_framebuffer != VK_NULL_HANDLE)
		SelfDestruct();

	vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &m_framebuffer);
}
}
