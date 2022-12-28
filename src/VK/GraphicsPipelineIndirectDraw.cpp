#include <GraphicsPipelineIndirectDraw.hpp>
#include <Shader.hpp>

GraphicsPipelineIndirectDraw::GraphicsPipelineIndirectDraw() noexcept
	: m_modelCount{ 0u }, m_counterBufferOffset{ 0u }, m_argumentBufferOffset{ 0u } {}

void GraphicsPipelineIndirectDraw::BindGraphicsPipeline(
	VkCommandBuffer graphicsCmdBuffer
) const noexcept {
	vkCmdBindPipeline(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->GetPipeline()
	);
}

void GraphicsPipelineIndirectDraw::DrawModels(
	VkCommandBuffer graphicsCmdBuffer, VkBuffer argumentBuffer, VkBuffer counterBuffer
) const noexcept {
	static constexpr auto strideSize =
		static_cast<std::uint32_t>(sizeof(VkDrawIndexedIndirectCommand));

	vkCmdDrawIndexedIndirectCount(
		graphicsCmdBuffer, argumentBuffer, m_argumentBufferOffset,
		counterBuffer, m_counterBufferOffset, m_modelCount, strideSize
	);
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineIndirectDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader
) const noexcept {
	auto vs = std::make_unique<Shader>(device);
	vs->CreateShader(device, shaderPath + L"VertexShader.spv");

	auto fs = std::make_unique<Shader>(device);
	fs->CreateShader(device, shaderPath + fragmentShader);

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateGraphicsPipeline(
		device, graphicsLayout, renderPass,
		VertexLayout()
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32_SFLOAT, 8u)
		.InitLayout(),
		vs->GetByteCode(), fs->GetByteCode()
	);

	return pso;
}

void GraphicsPipelineIndirectDraw::ConfigureGraphicsPipeline(
	const std::wstring& fragmentShader, std::uint32_t modelCount,
	std::uint32_t modelCountOffset, size_t counterIndex
) noexcept {
	m_modelCount = modelCount;

	m_argumentBufferOffset = sizeof(VkDrawIndexedIndirectCommand) * modelCountOffset;
	m_counterBufferOffset = sizeof(std::uint32_t) * 2u * counterIndex;
	m_fragmentShader = fragmentShader;
}

void GraphicsPipelineIndirectDraw::CreateGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath
) {
	m_graphicsPipeline = _createGraphicsPipeline(
		device, graphicsLayout, renderPass, shaderPath, m_fragmentShader
	);
}
