#include <VKPipelineObject.hpp>

VkPipelineObject::~VkPipelineObject() noexcept
{
	vkDestroyPipeline(m_device, m_pipeline, nullptr);
}

void VkPipelineObject::CreateGraphicsPipeline(const GraphicsPipelineBuilder& builder) noexcept
{
	vkCreateGraphicsPipelines(
		m_device, VK_NULL_HANDLE, 1u, builder.GetRef(), nullptr, &m_pipeline
	);
}

void VkPipelineObject::CreateComputePipeline(const ComputePipelineBuilder& builder) noexcept
{
	vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1u, builder.GetRef(), nullptr, &m_pipeline);
}

// Pipeline Builder Base
VkPipelineShaderStageCreateInfo PipelineBuilderBase::GetShaderStageInfo(
	VkShaderModule shader, VkShaderStageFlagBits shaderType
) noexcept {
	VkPipelineShaderStageCreateInfo shaderStage{
		.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage  = shaderType,
		.module = shader,
		.pName  = "main"
	};

	return shaderStage;
}

// Compute Pipeline Builder
ComputePipelineBuilder& ComputePipelineBuilder::SetComputeShaderStage(
	VkShaderModule computeShader
) noexcept {
	m_pipelineCreateInfo.stage = GetShaderStageInfo(computeShader, VK_SHADER_STAGE_COMPUTE_BIT);

	return *this;
}

// Graphics Pipeline Builder
GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetInputAssembler(
	const VertexLayout& vertexLayout,
	VkPrimitiveTopology topology /* = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST */,
	VkBool32 primitiveRestart /* = VK_FALSE */
) noexcept {
	m_vertexLayout                           = vertexLayout;
	m_pipelineCreateInfo.pVertexInputState   = m_vertexLayout.GetRef();
	m_pipelineCreateInfo.pInputAssemblyState = &m_inputAssemblerInfo;

	return *this;
}

void GraphicsPipelineBuilder::AddShaderStages() noexcept {
	m_pipelineCreateInfo.stageCount = static_cast<std::uint32_t>(std::size(m_shaderStagesInfo));
	m_pipelineCreateInfo.pStages    = std::data(m_shaderStagesInfo);
}


GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexShaderStage(
	VkShaderModule vertexShader, VkShaderModule fragmentShader
) noexcept {
	m_shaderStagesInfo.emplace_back(
		GetShaderStageInfo(vertexShader, VK_SHADER_STAGE_VERTEX_BIT)
	);
	m_shaderStagesInfo.emplace_back(
		GetShaderStageInfo(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	AddShaderStages();

	return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetMeshShaderStage(
	VkShaderModule meshShader, VkShaderModule fragmentShader,
	VkShaderModule taskShader /* = VK_NULL_HANDLE */
) noexcept {
	if (taskShader != VK_NULL_HANDLE)
		m_shaderStagesInfo.emplace_back(
			GetShaderStageInfo(taskShader, VK_SHADER_STAGE_TASK_BIT_EXT)
		);

	m_shaderStagesInfo.emplace_back(
		GetShaderStageInfo(meshShader, VK_SHADER_STAGE_MESH_BIT_EXT)
	);
	m_shaderStagesInfo.emplace_back(
		GetShaderStageInfo(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	AddShaderStages();

	return *this;
}

void GraphicsPipelineBuilder::UpdatePointers(bool inputAssembler) noexcept
{
	m_colourStateInfo.pAttachments           = &m_colourAttachmentState;
	m_dynamicStateInfo.pDynamicStates        = std::data(m_dynamicStates);
	m_pipelineCreateInfo.pViewportState      = &m_viewportInfo;
	m_pipelineCreateInfo.pRasterizationState = &m_rasterizationInfo;
	m_pipelineCreateInfo.pMultisampleState   = &m_multisampleInfo;
	m_pipelineCreateInfo.pDepthStencilState  = &m_depthStencilStateInfo;
	m_pipelineCreateInfo.pColorBlendState    = &m_colourStateInfo;
	m_pipelineCreateInfo.pDynamicState       = &m_dynamicStateInfo;

	if (inputAssembler)
	{
		m_pipelineCreateInfo.pVertexInputState = m_vertexLayout.GetRef();
		m_pipelineCreateInfo.pInputAssemblyState = &m_inputAssemblerInfo;
	}

	m_pipelineCreateInfo.pStages             = std::data(m_shaderStagesInfo);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddDynamicState(VkDynamicState dynamicState) noexcept
{
	m_dynamicStates.emplace_back(dynamicState);

	m_dynamicStateInfo.dynamicStateCount = static_cast<std::uint32_t>(std::size(m_dynamicStates));

	return *this;
}
