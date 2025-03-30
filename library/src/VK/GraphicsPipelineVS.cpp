#include <GraphicsPipelineVS.hpp>
#include <VkShader.hpp>

// Vertex Shader
static std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineVS(
	VkDevice device, VkPipelineLayout graphicsLayout,
	ShaderType binaryType, const std::wstring& shaderPath,
	const ExternalGraphicsPipeline& graphicsExtPipeline
) {
	auto vs              = std::make_unique<VkShader>(device);
	const bool vsSuccess = vs->Create(
		shaderPath + graphicsExtPipeline.GetVertexShader().GetNameWithExtension(binaryType)
	);

	auto fs              = std::make_unique<VkShader>(device);
	const bool fsSuccess = fs->Create(
		shaderPath + graphicsExtPipeline.GetFragmentShader().GetNameWithExtension(binaryType)
	);

	GraphicsPipelineBuilder builder{ graphicsLayout };

	builder.SetInputAssembler(
		VertexLayout{}
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32_SFLOAT, 8u)
		.InitLayout()
	);

	ConfigurePipelineBuilder(builder, graphicsExtPipeline);

	auto pso = std::make_unique<VkPipelineObject>(device);

	if (vsSuccess && fsSuccess)
	{
		builder.SetVertexStage(vs->Get(), fs->Get());

		pso->CreateGraphicsPipeline(builder);
	}

	return pso;
}

// Indirect Draw
std::unique_ptr<VkPipelineObject> GraphicsPipelineVSIndirectDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout,
	const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, s_shaderBytecodeType, shaderPath, graphicsExtPipeline
	);
}

// Individual Draw
std::unique_ptr<VkPipelineObject> GraphicsPipelineVSIndividualDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout,
	const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, s_shaderBytecodeType, shaderPath, graphicsExtPipeline
	);
}
