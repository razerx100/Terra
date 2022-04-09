#include <RenderPassManager.hpp>
#include <VKThrowMacros.hpp>

RenderPassManager::RenderPassManager(
	VkDevice device, VkFormat swapchainFormat
) : m_deviceRef(device), m_renderPass(VK_NULL_HANDLE) {
	CreateRenderPass(device, swapchainFormat);
}

RenderPassManager::~RenderPassManager() noexcept {
	vkDestroyRenderPass(m_deviceRef, m_renderPass, nullptr);
}

VkRenderPass RenderPassManager::GetRenderPass() const noexcept {
	return m_renderPass;
}

void RenderPassManager::CreateRenderPass(VkDevice device, VkFormat swapchainFormat) {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0u;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1u;
	subpassDesc.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0u;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0u;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1u;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1u;
	createInfo.pSubpasses = &subpassDesc;
	createInfo.dependencyCount = 1u;
	createInfo.pDependencies = &subpassDependency;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateRenderPass(device, &createInfo, nullptr, &m_renderPass)
	);
}
