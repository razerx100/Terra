#include <GraphicsPipelineVS.hpp>
#include <VkShader.hpp>

// Vertex Shader
static std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineVS(
	VkDevice device, VkPipelineLayout graphicsLayout,
	VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat,
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

	GraphicsPipelineBuilder builder{ graphicsLayout };

	builder.SetInputAssembler(
		VertexLayout{}
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32_SFLOAT, 8u)
		.InitLayout()
	).SetVertexStage(vs->Get(), fs->Get());

	// Don't think it will be that useful to have multiple colour attachment in the same pass?
	builder.AddColourAttachment(colourFormat);

	if (depthStencilFormat.depthFormat != VK_FORMAT_UNDEFINED)
		builder.SetDepthStencilState(
			DepthStencilStateBuilder{}.Enable(VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE),
			depthStencilFormat.depthFormat, depthStencilFormat.stencilFormat
		);

	auto pso = std::make_unique<VkPipelineObject>(device);

	if (vsSuccess && fsSuccess)
		pso->CreateGraphicsPipeline(builder);

	return pso;
}

// Indirect Draw
std::unique_ptr<VkPipelineObject> GraphicsPipelineVSIndirectDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout,
	VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, colourFormat, depthStencilFormat, s_shaderBytecodeType,
		shaderPath, fragmentShader, L"VertexShaderIndirect"
	);
}

// Individual Draw
std::unique_ptr<VkPipelineObject> GraphicsPipelineVSIndividualDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout,
	VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, colourFormat, depthStencilFormat, s_shaderBytecodeType,
		shaderPath, fragmentShader, L"VertexShaderIndividual"
	);
}
