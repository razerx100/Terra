#include <GraphicsPipelineMeshShader.hpp>
#include <VkShader.hpp>
#include <Exception.hpp>

#include <Terra.hpp>

void GraphicsPipelineMeshShader::ConfigureGraphicsPipeline(
	const std::wstring& fragmentShader
) noexcept {
	m_fragmentShader = fragmentShader;
}

void GraphicsPipelineMeshShader::AddModelDetails(
	std::uint32_t meshletCount, std::uint32_t meshletOffset, std::uint32_t modelIndex
) noexcept {
	ModelDetails modelDetails{
		.modelIndex = modelIndex,
		.meshletOffset = meshletOffset,
		.meshletCount = meshletCount
	};

	m_modelDetails.emplace_back(modelDetails);
}

void GraphicsPipelineMeshShader::DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept {
	for (const auto& modelDetail : m_modelDetails) {
		static constexpr std::uint32_t pushConstantSize =
			static_cast<std::uint32_t>(sizeof(std::uint32_t) * 2u);

		vkCmdPushConstants(
			graphicsCmdBuffer, m_graphicsLayout, VK_SHADER_STAGE_MESH_BIT_EXT, 0u,
			pushConstantSize, &modelDetail.modelIndex
		);
		s_vkCmdDrawMeshTasksEXT(graphicsCmdBuffer, modelDetail.meshletCount, 1u, 1u);
	}
}

void GraphicsPipelineMeshShader::AddPipelineLayout(VkPipelineLayout pipelineLayout) noexcept {
	m_graphicsLayout = pipelineLayout;
}

void GraphicsPipelineMeshShader::CreateGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath
) noexcept {
	m_graphicsPipeline = CreateGraphicsPipelineMS(
		device, graphicsLayout, renderPass, shaderPath, m_fragmentShader,
		L"MeshShader.spv"
	);

	AddPipelineLayout(graphicsLayout);
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineMeshShader::CreateGraphicsPipelineMS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader,
	const std::wstring& meshShader
) const noexcept {
	auto ms = std::make_unique<VkShader>(device);
	ms->CreateShader(device, shaderPath + meshShader);

	auto fs = std::make_unique<VkShader>(device);
	fs->CreateShader(device, shaderPath + fragmentShader);

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateGraphicsPipelineMS(
		device, graphicsLayout, renderPass, ms->GetShaderModule(), fs->GetShaderModule()
	);

	return pso;
}

void GraphicsPipelineMeshShader::AddRequiredExtensionFunctions() noexcept {
	Terra::Get().DeviceExtensionLoader().AddFunctionPTR(s_DrawMeshTasksName);
}

void GraphicsPipelineMeshShader::RetrieveExtensionFunctions() {
	Terra::Get().DeviceExtensionLoader().GetFunctionPointer(
		s_DrawMeshTasksName, s_vkCmdDrawMeshTasksEXT
	);

	if (!s_vkCmdDrawMeshTasksEXT)
		throw Exception(
			"Function Retrieval Error", "Couldn't load the function "s + s_DrawMeshTasksName
		);
}
