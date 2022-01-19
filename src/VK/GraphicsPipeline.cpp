#include <GraphicsPipeline.hpp>
#include <VKThrowMacros.hpp>
#include <InstanceManager.hpp>
#include <CommonPipelineObjects.hpp>

GraphicsPipeline::GraphicsPipeline(VkDevice device) noexcept : m_deviceRef(device) {}

GraphicsPipeline::~GraphicsPipeline() noexcept {
	vkDestroyPipeline(m_deviceRef, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_deviceRef, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_deviceRef, m_renderPass, nullptr);
}

void GraphicsPipeline::CreatePipelineLayout() {
	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.setLayoutCount = 0u;
	createInfo.pSetLayouts = nullptr;
	createInfo.pushConstantRangeCount = 0u;
	createInfo.pPushConstantRanges = nullptr;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreatePipelineLayout(m_deviceRef, &createInfo, nullptr, &m_pipelineLayout)
	);
}

void GraphicsPipeline::CreateRenderPass() {
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = SwapChainInst::GetRef()->GetSwapFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0u;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1u;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1u;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1u;
	createInfo.pSubpasses = &subpass;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateRenderPass(m_deviceRef, &createInfo, nullptr, &m_renderPass)
	);
}

void GraphicsPipeline::CreateGraphicsPipeline() {
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	PopulateVertexInputStateCreateInfo(vertexInputInfo);

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	PopulateInputAssemblyStateCreateInfo(inputAssemblyInfo);

	VkViewport viewport[1u];
	VkExtent2D swapExtent = SwapChainInst::GetRef()->GetSwapExtent();
	PopulateViewport(viewport[0u], swapExtent.width, swapExtent.height);

	VkRect2D scissorRect[1u];
	PopulateScissorRect(scissorRect[0u], swapExtent.width, swapExtent.height);

	VkPipelineViewportStateCreateInfo viewportInfo;
	PopulateViewportStateCreateInfo(viewportInfo, viewport, scissorRect);

	VkPipelineRasterizationStateCreateInfo rasterizerInfo;
	PopulateRasterizationStateCreateInfo(rasterizerInfo);

	VkPipelineMultisampleStateCreateInfo mulisamplingInfo;
	PopulateMultisampleStateCreateInfo(mulisamplingInfo);

	VkPipelineColorBlendAttachmentState colorBlendAttachment[1u];
	PopulateColorBlendAttachmentState(colorBlendAttachment[0u]);

	VkPipelineColorBlendStateCreateInfo colorBlending;
	PopulateColorBlendStateCreateInfo(colorBlending, colorBlendAttachment);

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicInfo;
	PopulateDynamicStateCreateInfo(dynamicInfo, dynamicStates);

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 0u;
	pipelineInfo.pStages = nullptr;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizerInfo;
	pipelineInfo.pMultisampleState = &mulisamplingInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicInfo;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0u;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateGraphicsPipelines(
			m_deviceRef, VK_NULL_HANDLE, 1u, &pipelineInfo, nullptr, &m_graphicsPipeline
		)
	);
}
