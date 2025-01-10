#include <GraphicsPipelineMS.hpp>
#include <VkShader.hpp>

std::unique_ptr<VkPipelineObject> GraphicsPipelineMS::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout,
	VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat,
	const std::wstring& shaderPath, const ShaderName& fragmentShader
) const {
	constexpr const wchar_t* meshShaderName = L"MeshShaderMSIndividual";
	constexpr const wchar_t* taskShaderName = L"MeshShaderTSIndividual";

	return CreateGraphicsPipelineMS(
		device, graphicsLayout, colourFormat, depthStencilFormat, s_shaderBytecodeType, shaderPath,
		fragmentShader, meshShaderName, taskShaderName
	);
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineMS::CreateGraphicsPipelineMS(
	VkDevice device, VkPipelineLayout graphicsLayout,
	VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat,
	ShaderType binaryType, const std::wstring& shaderPath, const ShaderName& fragmentShader,
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
		shaderPath + fragmentShader.GetNameWithExtension(binaryType)
	);

	GraphicsPipelineBuilder builder{ graphicsLayout };

	// Don't think it will be that useful to have multiple colour attachment in the same pass?
	builder.AddColourAttachment(colourFormat);

	if (depthStencilFormat.depthFormat != VK_FORMAT_UNDEFINED)
		builder.SetDepthStencilState(
			DepthStencilStateBuilder{}.Enable(VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE),
			depthStencilFormat.depthFormat, depthStencilFormat.stencilFormat
		);

	auto pso = std::make_unique<VkPipelineObject>(device);

	if (msSuccess && fsSuccess && tsSuccess)
	{
		builder.SetTaskStage(ts->Get()).SetMeshStage(ms->Get(), fs->Get());

		pso->CreateGraphicsPipeline(builder);
	}

	return pso;
}
