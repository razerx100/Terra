#include <GraphicsPipelineVS.hpp>
#include <VkShader.hpp>

// Vertex Shader
static std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineVS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	ShaderType binaryType, const std::wstring& shaderPath,
	const ShaderName& fragmentShader, const ShaderName& vertexShader
) {
	auto vs              = std::make_unique<VkShader>(device);
	const bool vsSuccess = vs->Create(
		shaderPath + vertexShader.GetNameWithExtension(binaryType)
	);

	auto fs              = std::make_unique<VkShader>(device);
	const bool fsSuccess = fs->Create(
		shaderPath + fragmentShader.GetNameWithExtension(binaryType)
	);

	auto pso = std::make_unique<VkPipelineObject>(device);

	if (vsSuccess && fsSuccess)
		pso->CreateGraphicsPipeline(
			GraphicsPipelineBuilder{ graphicsLayout, renderPass }
			.SetInputAssembler(
				VertexLayout{}
				.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
				.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
				.AddInput(VK_FORMAT_R32G32_SFLOAT, 8u)
				.InitLayout()
			)
			.SetVertexStage(vs->Get(), fs->Get())
		);

	return pso;
}

// Indirect Draw
std::unique_ptr<VkPipelineObject> GraphicsPipelineVSIndirectDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, renderPass, s_shaderBytecodeType,
		shaderPath, fragmentShader, L"VertexShaderIndirect"
	);
}

// Individual Draw
std::unique_ptr<VkPipelineObject> GraphicsPipelineVSIndividualDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, renderPass, s_shaderBytecodeType,
		shaderPath, fragmentShader, L"VertexShaderIndividual"
	);
}
