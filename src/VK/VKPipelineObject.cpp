#include <VKPipelineObject.hpp>

VkPipelineObject::VkPipelineObject(VkDevice device) noexcept
	: m_deviceRef{ device }, m_pipeline{ VK_NULL_HANDLE } {}

void VkPipelineObject::CreateGraphicsPipelineVS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const VertexLayout& vertexInput, VkShaderModule vertexShader,
	VkShaderModule fragmentShader
) noexcept {
	GraphicsPipelineCreateInfo pipelineInfo{ graphicsLayout, renderPass };
	pipelineInfo.SetInputAssembler(vertexInput);
	pipelineInfo.SetVertexShaderStage(vertexShader, fragmentShader);

	vkCreateGraphicsPipelines(
		device, VK_NULL_HANDLE, 1u, pipelineInfo.GetGraphicsCreateInfoRef(), nullptr,
		&m_pipeline
	);
}

void VkPipelineObject::CreateGraphicsPipelineMS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	VkShaderModule meshShader
) noexcept {
	GraphicsPipelineCreateInfo pipelineInfo{ graphicsLayout, renderPass };
	pipelineInfo.SetMeshShaderStage(meshShader);

	vkCreateGraphicsPipelines(
		device, VK_NULL_HANDLE, 1u, pipelineInfo.GetGraphicsCreateInfoRef(), nullptr,
		&m_pipeline
	);
}

void VkPipelineObject::CreateComputePipeline(
	VkDevice device, VkPipelineLayout computeLayout, VkShaderModule computeShader
) noexcept {
	VkPipelineShaderStageCreateInfo shaderStage = GetShaderStageInfo(
		computeShader, VK_SHADER_STAGE_COMPUTE_BIT
	);

	VkComputePipelineCreateInfo pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = shaderStage,
		.layout = computeLayout,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	vkCreateComputePipelines(device, VK_NULL_HANDLE, 1u, &pipelineInfo, nullptr, &m_pipeline);
}

VkPipelineObject::~VkPipelineObject() noexcept {
	vkDestroyPipeline(m_deviceRef, m_pipeline, nullptr);
}

VkPipelineShaderStageCreateInfo VkPipelineObject::GetShaderStageInfo(
	VkShaderModule shader, VkShaderStageFlagBits shaderType
) noexcept {
	VkPipelineShaderStageCreateInfo shaderStage{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = shaderType,
		.module = shader,
		.pName = "main"
	};

	return shaderStage;
}

VkPipeline VkPipelineObject::GetPipeline() const noexcept {
	return m_pipeline;
}

// Graphics Pipeline Create Info
VkPipelineObject::GraphicsPipelineCreateInfo::GraphicsPipelineCreateInfo(
	VkPipelineLayout graphicsLayout, VkRenderPass renderPass
) noexcept : m_inputAssemblerInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	.primitiveRestartEnable = VK_FALSE
}, m_viewportInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	.viewportCount = 1u,
	.pViewports = nullptr,
	.scissorCount = 1u,
	.pScissors = nullptr
}, m_rasterizationInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	.depthClampEnable = VK_FALSE,
	.rasterizerDiscardEnable = VK_FALSE,
	.polygonMode = VK_POLYGON_MODE_FILL,
	.cullMode = VK_CULL_MODE_BACK_BIT,
	.frontFace = VK_FRONT_FACE_CLOCKWISE,
	.depthBiasEnable = VK_FALSE,
	.depthBiasConstantFactor = 0.f,
	.depthBiasClamp = 0.f,
	.depthBiasSlopeFactor = 0.f,
	.lineWidth = 1.f
}, m_multisampleInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	.sampleShadingEnable = VK_FALSE,
	.minSampleShading = 1.f,
	.pSampleMask = nullptr,
	.alphaToCoverageEnable = VK_FALSE,
	.alphaToOneEnable = VK_FALSE
}, m_colourAttachmentState{
	.blendEnable = VK_FALSE,
	.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
	.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
	.colorBlendOp = VK_BLEND_OP_ADD,
	.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
	.alphaBlendOp = VK_BLEND_OP_ADD,
	.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
}, m_colourStateInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	.logicOpEnable = VK_FALSE,
	.logicOp = VK_LOGIC_OP_COPY,
	.attachmentCount = 1u,
	.pAttachments = &m_colourAttachmentState,
	.blendConstants = { 0.f, 0.f, 0.f, 0.f }
}, m_dynamicStates{
	{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }
}, m_dynamicStateInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	.dynamicStateCount = 2u,
	.pDynamicStates = std::data(m_dynamicStates)
}, m_depthStencilStateInfo{
	.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	.depthTestEnable = VK_TRUE,
	.depthWriteEnable = VK_TRUE,
	.depthCompareOp = VK_COMPARE_OP_LESS,
	.depthBoundsTestEnable = VK_FALSE,
	.stencilTestEnable = VK_FALSE,
	.front = {},
	.back = {},
	.minDepthBounds = 0.f,
	.maxDepthBounds = 1.f
}, m_pipelineCreateInfo{
	.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	.pViewportState = &m_viewportInfo,
	.pRasterizationState = &m_rasterizationInfo,
	.pMultisampleState = &m_multisampleInfo,
	.pDepthStencilState = &m_depthStencilStateInfo,
	.pColorBlendState = &m_colourStateInfo,
	.pDynamicState = &m_dynamicStateInfo,
	.layout = graphicsLayout,
	.renderPass = renderPass,
	.subpass = 0u,
	.basePipelineHandle = VK_NULL_HANDLE,
	.basePipelineIndex = -1
} {}

void VkPipelineObject::GraphicsPipelineCreateInfo::SetInputAssembler(
	const VertexLayout& vertexLayout
) noexcept {
	m_vertexLayout = vertexLayout;
	m_pipelineCreateInfo.pVertexInputState = m_vertexLayout.GetInputInfo();

	m_pipelineCreateInfo.pInputAssemblyState = &m_inputAssemblerInfo;
}

void VkPipelineObject::GraphicsPipelineCreateInfo::AddShaderStages() noexcept {
	m_pipelineCreateInfo.stageCount =
		static_cast<std::uint32_t>(std::size(m_shaderStagesInfo));
	m_pipelineCreateInfo.pStages = std::data(m_shaderStagesInfo);
}

void VkPipelineObject::GraphicsPipelineCreateInfo::SetVertexShaderStage(
	VkShaderModule vertexShader, VkShaderModule fragmentShader
) noexcept {
	m_shaderStagesInfo.emplace_back(
		GetShaderStageInfo(vertexShader, VK_SHADER_STAGE_VERTEX_BIT)
	);
	m_shaderStagesInfo.emplace_back(
		GetShaderStageInfo(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	AddShaderStages();
}

void VkPipelineObject::GraphicsPipelineCreateInfo::SetMeshShaderStage(
	VkShaderModule meshShader
) noexcept {
	m_shaderStagesInfo.emplace_back(
		GetShaderStageInfo(meshShader, VK_SHADER_STAGE_MESH_BIT_EXT)
	);

	AddShaderStages();
}

VkGraphicsPipelineCreateInfo const* VkPipelineObject::GraphicsPipelineCreateInfo
::GetGraphicsCreateInfoRef() const noexcept {
	return &m_pipelineCreateInfo;
}
