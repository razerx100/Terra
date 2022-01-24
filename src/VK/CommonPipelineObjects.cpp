#include <CommonPipelineObjects.hpp>

VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyStateCreateInfo() noexcept {
	VkPipelineInputAssemblyStateCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	createInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	createInfo.primitiveRestartEnable = VK_FALSE;

	return createInfo;
}

VkPipelineViewportStateCreateInfo GetViewportStateCreateInfo() noexcept {
	VkPipelineViewportStateCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	createInfo.pViewports = nullptr;
	createInfo.viewportCount = 1u;
	createInfo.scissorCount = 1u;
	createInfo.pScissors = nullptr;

	return createInfo;
}

void PopulateRasterizationStateCreateInfo(
	VkPipelineRasterizationStateCreateInfo& createInfo
) noexcept {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	createInfo.depthClampEnable = VK_FALSE;
	createInfo.rasterizerDiscardEnable = VK_FALSE;
	createInfo.polygonMode = VK_POLYGON_MODE_FILL;
	createInfo.lineWidth = 1.0f;
	createInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	createInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	createInfo.depthBiasEnable = VK_FALSE;
	createInfo.depthBiasConstantFactor = 0.0f;
	createInfo.depthBiasClamp = 0.0f;
	createInfo.depthBiasSlopeFactor = 0.0f;
}

void PopulateMultisampleStateCreateInfo(
	VkPipelineMultisampleStateCreateInfo& createInfo
) noexcept {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	createInfo.sampleShadingEnable = VK_FALSE;
	createInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.minSampleShading = 1.0f;
	createInfo.pSampleMask = nullptr;
	createInfo.alphaToCoverageEnable = VK_FALSE;
	createInfo.alphaToOneEnable = VK_FALSE;
}

void PopulateColorBlendAttachmentState(
	VkPipelineColorBlendAttachmentState& createInfo
) noexcept {
	createInfo = {};
	createInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	createInfo.blendEnable = VK_FALSE;
	createInfo.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	createInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	createInfo.colorBlendOp = VK_BLEND_OP_ADD;
	createInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	createInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	createInfo.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PopulateColorBlendStateCreateInfo(
	VkPipelineColorBlendStateCreateInfo& createInfo,
	const std::span<VkPipelineColorBlendAttachmentState> attachmentStates
) noexcept {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	createInfo.logicOpEnable = VK_FALSE;
	createInfo.logicOp = VK_LOGIC_OP_COPY;
	createInfo.attachmentCount = static_cast<std::uint32_t>(attachmentStates.size());
	createInfo.pAttachments = attachmentStates.data();
	createInfo.blendConstants[0u] = 0.0f;
	createInfo.blendConstants[1u] = 0.0f;
	createInfo.blendConstants[2u] = 0.0f;
	createInfo.blendConstants[3u] = 0.0f;
}

void PopulateDynamicStateCreateInfo(
	VkPipelineDynamicStateCreateInfo& createInfo,
	const std::span<VkDynamicState> dynamicStates
) noexcept {
	createInfo = {};
	createInfo.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
	createInfo.pDynamicStates = dynamicStates.data();
}
