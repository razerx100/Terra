#include <GraphicsPipelineMeshShader.hpp>
#include <VkShader.hpp>

std::unique_ptr<VkPipelineObject> GraphicsPipelineMeshShader::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) const {
	constexpr const wchar_t* meshShaderName = L"MeshShaderIndividual";
	constexpr const wchar_t* taskShaderName = L"MeshShaderTSIndividual";

	if (m_useTaskShader)
		return CreateGraphicsPipelineMS(
			device, graphicsLayout, renderPass, s_shaderBytecodeType, shaderPath, fragmentShader,
			meshShaderName, taskShaderName
		);
	else
		return CreateGraphicsPipelineMS(
			device, graphicsLayout, renderPass, s_shaderBytecodeType, shaderPath, fragmentShader,
			meshShaderName
		);
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineMeshShader::CreateGraphicsPipelineMS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	ShaderType binaryType, const std::wstring& shaderPath, const ShaderName& fragmentShader,
	const ShaderName& meshShader, const ShaderName& taskShader /* = {} */
) {
	auto ms              = std::make_unique<VkShader>(device);
	const bool msSuccess = ms->Create(
		shaderPath + meshShader.GetNameWithExtension(binaryType)
	);

	auto fs              = std::make_unique<VkShader>(device);
	const bool fsSuccess = fs->Create(
		shaderPath + fragmentShader.GetNameWithExtension(binaryType)
	);

	GraphicsPipelineBuilder builder{ graphicsLayout, renderPass };

	bool tsSuccess = true;

	if (!std::empty(taskShader))
	{
		auto ts   = std::make_unique<VkShader>(device);
		tsSuccess = ts->Create(
			shaderPath + taskShader.GetNameWithExtension(binaryType)
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
