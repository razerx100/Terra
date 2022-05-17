#include <RenderPassManager.hpp>
#include <VKThrowMacros.hpp>

RenderPassManager::RenderPassManager(
	VkDevice device,
	VkFormat swapchainFormat, VkFormat depthFormat
) : m_deviceRef(device), m_renderPass(VK_NULL_HANDLE) {
	CreateRenderPass(device, swapchainFormat, depthFormat);
}

RenderPassManager::~RenderPassManager() noexcept {
	vkDestroyRenderPass(m_deviceRef, m_renderPass, nullptr);
}

VkRenderPass RenderPassManager::GetRenderPass() const noexcept {
	return m_renderPass;
}

void RenderPassManager::CreateRenderPass(
	VkDevice device,
	VkFormat swapchainFormat, VkFormat depthFormat
) {
	VkAttachmentReference colourAttachmentRef = {};
	colourAttachmentRef.attachment = 0u;
	colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1u;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1u;
	subpassDesc.pColorAttachments = &colourAttachmentRef;
	subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0u;
	subpassDependency.srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.srcAccessMask = 0u;
	subpassDependency.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription colourAttachment = GetColourAttachment(swapchainFormat);
	VkAttachmentDescription depthAttachment = GetDepthAttachment(depthFormat);

	VkAttachmentDescription attachments[] = { colourAttachment, depthAttachment };

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<std::uint32_t>(std::size(attachments));
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1u;
	createInfo.pSubpasses = &subpassDesc;
	createInfo.dependencyCount = 1u;
	createInfo.pDependencies = &subpassDependency;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateRenderPass(device, &createInfo, nullptr, &m_renderPass)
	);
}

VkAttachmentDescription RenderPassManager::GetColourAttachment(
	VkFormat colourFormat
) const noexcept {
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.format = colourFormat;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	return colourAttachment;
}

VkAttachmentDescription RenderPassManager::GetDepthAttachment(
	VkFormat depthFormat
) const noexcept {
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	return depthAttachment;
}
