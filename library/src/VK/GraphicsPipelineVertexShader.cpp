#include <GraphicsPipelineVertexShader.hpp>
#include <VkShader.hpp>

// Vertex Shader
std::unique_ptr<VkPipelineObject> GraphicsPipelineVertexShader::CreateGraphicsPipelineVS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader,
	const ShaderName& vertexShader
) {
	auto vs              = std::make_unique<VkShader>(device);
	const bool vsSuccess = vs->Create(
		shaderPath + vertexShader.GetNameWithExtension(s_shaderBytecodeType)
	);

	auto fs              = std::make_unique<VkShader>(device);
	const bool fsSuccess = fs->Create(
		shaderPath + fragmentShader.GetNameWithExtension(s_shaderBytecodeType)
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

void GraphicsPipelineVertexShader::Create(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) {
	m_fragmentShader = fragmentShader;

	m_graphicsPipeline = _createGraphicsPipeline(
		device, graphicsLayout, renderPass, shaderPath, m_fragmentShader
	);
}

// Indirect Draw
std::unique_ptr<VkPipelineObject> GraphicsPipelineIndirectDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, renderPass, shaderPath, fragmentShader,
		L"VertexShaderIndirect"
	);
}

// Individual Draw
std::unique_ptr<VkPipelineObject> GraphicsPipelineIndividualDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) const {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, renderPass, shaderPath, fragmentShader,
		L"VertexShaderIndividual"
	);
}
