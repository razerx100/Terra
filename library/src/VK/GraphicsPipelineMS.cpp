#include <GraphicsPipelineMS.hpp>
#include <VkShader.hpp>

std::unique_ptr<VkPipelineObject> GraphicsPipelineMS::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout,
	const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
) const {
	constexpr const wchar_t* meshShaderName = L"MeshShaderMSIndividual";
	constexpr const wchar_t* taskShaderName = L"MeshShaderTSIndividual";

	return CreateGraphicsPipelineMS(
		device, graphicsLayout, s_shaderBytecodeType, shaderPath, graphicsExtPipeline,
		meshShaderName, taskShaderName
	);
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineMS::CreateGraphicsPipelineMS(
	VkDevice device, VkPipelineLayout graphicsLayout,
	ShaderType binaryType, const std::wstring& shaderPath,
	const ExternalGraphicsPipeline& graphicsExtPipeline,
	const ShaderName& meshShader, const ShaderName& taskShader
) {
	auto ms              = std::make_unique<VkShader>(device);
	const bool msSuccess = ms->Create(
		shaderPath + meshShader.GetNameWithExtension(binaryType)
	);

	auto ts              = std::make_unique<VkShader>(device);
	const bool tsSuccess = ts->Create(
		shaderPath + taskShader.GetNameWithExtension(binaryType)
	);

	auto fs              = std::make_unique<VkShader>(device);
	const bool fsSuccess = fs->Create(
		shaderPath + graphicsExtPipeline.GetFragmentShader().GetNameWithExtension(binaryType)
	);

	GraphicsPipelineBuilder builder{ graphicsLayout };

	ConfigurePipelineBuilder(builder, graphicsExtPipeline);

	auto pso = std::make_unique<VkPipelineObject>(device);

	if (msSuccess && fsSuccess && tsSuccess)
	{
		builder.SetTaskStage(ts->Get()).SetMeshStage(ms->Get(), fs->Get());

		pso->CreateGraphicsPipeline(builder);
	}

	return pso;
}
