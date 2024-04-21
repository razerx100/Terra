#include <GraphicsPipelineMeshShader.hpp>
#include <VkShader.hpp>

void GraphicsPipelineMeshShader::Create(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader
) noexcept {
	if (m_useTaskShader)
		m_graphicsPipeline = CreateGraphicsPipelineMS(
			device, graphicsLayout, renderPass, shaderPath, fragmentShader,
			L"MeshShader.spv", L"TaskShader.spv"
		);
	else
		m_graphicsPipeline = CreateGraphicsPipelineMS(
			device, graphicsLayout, renderPass, shaderPath, fragmentShader,
			L"MeshShader.spv"
		);
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineMeshShader::CreateGraphicsPipelineMS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader,
	const std::wstring& meshShader, const std::wstring& taskShader /* = {} */
) noexcept {
	auto ms = std::make_unique<VkShader>(device);
	ms->Create(shaderPath + meshShader);

	auto fs = std::make_unique<VkShader>(device);
	fs->Create(shaderPath + fragmentShader);

	GraphicsPipelineBuilder builder{ graphicsLayout, renderPass };

	if (!std::empty(taskShader))
	{
		auto ts = std::make_unique<VkShader>(device);
		ts->Create(shaderPath + taskShader);

		builder.SetTaskStage(ts->Get());
	}

	builder.SetMeshStage(ms->Get(), fs->Get());

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateGraphicsPipeline(builder);

	return pso;
}
