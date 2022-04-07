#include <PipelineObjectGFX.hpp>
#include <VKThrowMacros.hpp>

PipelineObjectGFX::PipelineObjectGFX(
	VkDevice device,
	VkPipelineLayout layout,
	VkRenderPass renderPass,
	const VkPipelineVertexInputStateCreateInfo* vertexInput,
	VkShaderModule vertexShader,
	VkShaderModule fragmentShader
) : m_deviceRef(device), m_graphicsPipeline(VK_NULL_HANDLE) {

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pViewports = nullptr;
	viewportState.viewportCount = 1u;
	viewportState.scissorCount = 1u;
	viewportState.pScissors = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colourBlendAttachment = {};
	colourBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colourBlendAttachment.blendEnable = VK_FALSE;
	colourBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colourBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colourBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colourBlending = {};
	colourBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlending.logicOpEnable = VK_FALSE;
	colourBlending.logicOp = VK_LOGIC_OP_COPY;
	colourBlending.attachmentCount = 1u;
	colourBlending.pAttachments = &colourBlendAttachment;
	colourBlending.blendConstants[0u] = 0.0f;
	colourBlending.blendConstants[1u] = 0.0f;
	colourBlending.blendConstants[2u] = 0.0f;
	colourBlending.blendConstants[3u] = 0.0f;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2u;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineShaderStageCreateInfo shaderStages[2] = {};

	shaderStages[0u].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0u].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0u].module = vertexShader;
	shaderStages[0u].pName = "main";

	shaderStages[1u].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1u].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1u].module = fragmentShader;
	shaderStages[1u].pName = "main";

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2u;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colourBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0u;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateGraphicsPipelines(
			device, VK_NULL_HANDLE, 1u, &pipelineInfo, nullptr, &m_graphicsPipeline
		)
	);
}

PipelineObjectGFX::~PipelineObjectGFX() noexcept {
	vkDestroyPipeline(m_deviceRef, m_graphicsPipeline, nullptr);
}

VkPipeline PipelineObjectGFX::GetPipelineObject() const noexcept {
	return m_graphicsPipeline;
}
