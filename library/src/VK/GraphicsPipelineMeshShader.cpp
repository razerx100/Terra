#include <GraphicsPipelineMeshShader.hpp>
#include <VkShader.hpp>

void GraphicsPipelineMeshShader::Create(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) {
	m_fragmentShader = fragmentShader;

	constexpr const wchar_t* meshShaderName = L"MeshShaderIndividual";
	constexpr const wchar_t* taskShaderName = L"MeshShaderTSIndividual";

	if (m_useTaskShader)
		m_graphicsPipeline = CreateGraphicsPipelineMS(
			device, graphicsLayout, renderPass, shaderPath, m_fragmentShader,
			meshShaderName, taskShaderName
		);
	else
		m_graphicsPipeline = CreateGraphicsPipelineMS(
			device, graphicsLayout, renderPass, shaderPath, m_fragmentShader,
			meshShaderName
		);
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineMeshShader::CreateGraphicsPipelineMS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader,
	const ShaderName& meshShader, const ShaderName& taskShader /* = {} */
) {
	auto ms              = std::make_unique<VkShader>(device);
	const bool msSuccess = ms->Create(
		shaderPath + meshShader.GetNameWithExtension(s_shaderBytecodeType)
	);

	auto fs              = std::make_unique<VkShader>(device);
	const bool fsSuccess = fs->Create(
		shaderPath + fragmentShader.GetNameWithExtension(s_shaderBytecodeType)
	);

	GraphicsPipelineBuilder builder{ graphicsLayout, renderPass };

	bool tsSuccess = true;

	if (!std::empty(taskShader))
	{
		auto ts   = std::make_unique<VkShader>(device);
		tsSuccess = ts->Create(
			shaderPath + taskShader.GetNameWithExtension(s_shaderBytecodeType)
		);

		if (tsSuccess)
			builder.SetTaskStage(ts->Get());
	}

	auto pso = std::make_unique<VkPipelineObject>(device);

	if (msSuccess && fsSuccess && tsSuccess)
	{
		builder.SetMeshStage(ms->Get(), fs->Get());
		pso->CreateGraphicsPipeline(builder);
	}

	return pso;
}
