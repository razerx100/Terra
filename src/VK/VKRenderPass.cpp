#include <VKRenderPass.hpp>

VKRenderPass::VKRenderPass(VkDevice device) noexcept
	: m_deviceRef{ device }, m_renderPass{ VK_NULL_HANDLE } {}

VKRenderPass::~VKRenderPass() noexcept {
	vkDestroyRenderPass(m_deviceRef, m_renderPass, nullptr);
}

VkRenderPass VKRenderPass::GetRenderPass() const noexcept {
	return m_renderPass;
}

void VKRenderPass::CreateRenderPass(
	VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat
) {
	VkAttachmentReference colourAttachmentRef{
		.attachment = 0u,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depthAttachmentRef{
		.attachment = 1u,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpassDesc{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1u,
		.pColorAttachments = &colourAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef
	};

	VkSubpassDependency subpassDependency{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0u,
		.srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0u,
		.dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
	};

	VkAttachmentDescription colourAttachment = GetColourAttachment(swapchainFormat);
	VkAttachmentDescription depthAttachment = GetDepthAttachment(depthFormat);

	VkAttachmentDescription attachments[] { colourAttachment, depthAttachment };

	VkRenderPassCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<std::uint32_t>(std::size(attachments)),
		.pAttachments = attachments,
		.subpassCount = 1u,
		.pSubpasses = &subpassDesc,
		.dependencyCount = 1u,
		.pDependencies = &subpassDependency
	};

	vkCreateRenderPass(device, &createInfo, nullptr, &m_renderPass);
}

VkAttachmentDescription VKRenderPass::GetColourAttachment(
	VkFormat colourFormat
) const noexcept {
	VkAttachmentDescription colourAttachment{
		.format = colourFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	return colourAttachment;
}

VkAttachmentDescription VKRenderPass::GetDepthAttachment(
	VkFormat depthFormat
) const noexcept {
	VkAttachmentDescription depthAttachment{
		.format = depthFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	return depthAttachment;
}
