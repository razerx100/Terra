#include <VKRenderPass.hpp>
#include <ranges>
#include <algorithm>

namespace Terra
{
// RenderPass builder
RenderPassBuilder& RenderPassBuilder::AddColourAttachment(VkFormat format) noexcept
{
	m_colourAttachmentRefs.emplace_back(
		VkAttachmentReference{
			.attachment = static_cast<std::uint32_t>(std::size(m_attachments)),
			.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		}
	);
	m_attachments.emplace_back(GetColourAttachment(format));

	m_subpassDependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Src is also output here, because the second pass would start from the colour output bit.
	m_subpassDependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	m_subpassDependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	return *this;
}

RenderPassBuilder& RenderPassBuilder::AddDepthAttachment(VkFormat format) noexcept
{
	m_depthAttachmentRefs.emplace_back(
		VkAttachmentReference{
			.attachment = static_cast<std::uint32_t>(std::size(m_attachments)),
			.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	);
	m_attachments.emplace_back(GetDepthAttachment(format));

	m_subpassDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	// Dst is also Early Fragement, since Depth Write would be done on the start of the second pass.
	m_subpassDependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	m_subpassDependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	return *this;
}

RenderPassBuilder& RenderPassBuilder::Build() noexcept
{
	m_subpassDesc.colorAttachmentCount    = static_cast<std::uint32_t>(
		std::size(m_colourAttachmentRefs)
	);
	m_subpassDesc.pColorAttachments       = std::data(m_colourAttachmentRefs);
	m_subpassDesc.pDepthStencilAttachment = std::data(m_depthAttachmentRefs);

	m_createInfo.attachmentCount = static_cast<std::uint32_t>(std::size(m_attachments));
	m_createInfo.pAttachments    = std::data(m_attachments);

	return *this;
}

VkAttachmentDescription RenderPassBuilder::GetColourAttachment(VkFormat format) noexcept
{
	return VkAttachmentDescription{
		.format         = format,
		.samples        = VK_SAMPLE_COUNT_1_BIT,
		.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
}

VkAttachmentDescription RenderPassBuilder::GetDepthAttachment(VkFormat format) noexcept
{
	return VkAttachmentDescription{
		.format         = format,
		.samples        = VK_SAMPLE_COUNT_1_BIT,
		.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
}

void RenderPassBuilder::UpdatePointers() noexcept
{
	m_subpassDesc.pColorAttachments       = std::data(m_colourAttachmentRefs);
	m_subpassDesc.pDepthStencilAttachment = std::data(m_depthAttachmentRefs);

	m_createInfo.pAttachments  = std::data(m_attachments);
	m_createInfo.pSubpasses    = &m_subpassDesc;
	m_createInfo.pDependencies = &m_subpassDependency;
}

// RenderPass
VKRenderPass::~VKRenderPass() noexcept
{
	SelfDestruct();
}

void VKRenderPass::SelfDestruct() noexcept
{
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
}

void VKRenderPass::Create(const RenderPassBuilder& renderPassBuilder)
{
	if (m_renderPass != VK_NULL_HANDLE)
		SelfDestruct();

	vkCreateRenderPass(m_device, renderPassBuilder.GetRef(), nullptr, &m_renderPass);
}

void VKRenderPass::BeginPass(
	VkCommandBuffer graphicsCmdBuffer, VkFramebuffer frameBuffer, VkExtent2D swapchainExtent,
	std::span<VkClearValue> clearValues
) {
	VkRenderPassBeginInfo renderPassInfo
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass      = m_renderPass,
		.framebuffer     = frameBuffer,
		.renderArea      = VkRect2D{ .offset = VkOffset2D{ 0, 0 }, .extent = swapchainExtent },
		.clearValueCount = static_cast<std::uint32_t>(std::size(clearValues)),
		.pClearValues    = std::data(clearValues)
	};

	vkCmdBeginRenderPass(graphicsCmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VKRenderPass::EndPass(VkCommandBuffer graphicsCmdBuffer)
{
	vkCmdEndRenderPass(graphicsCmdBuffer);
}
}
