#include <GraphicsPipelineVertexShader.hpp>
#include <VkShader.hpp>

// Vertex Shader
std::unique_ptr<VkPipelineObject> GraphicsPipelineVertexShader::CreateGraphicsPipelineVS(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader,
	const std::wstring& vertexShader
) const noexcept {
	auto vs = std::make_unique<VkShader>(device);
	vs->CreateShader(device, shaderPath + vertexShader);

	auto fs = std::make_unique<VkShader>(device);
	fs->CreateShader(device, shaderPath + fragmentShader);

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateGraphicsPipelineVS(
		device, graphicsLayout, renderPass,
		VertexLayout()
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32_SFLOAT, 8u)
		.InitLayout(),
		vs->GetShaderModule(), fs->GetShaderModule()
	);

	return pso;
}

void GraphicsPipelineVertexShader::CreateGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath
) noexcept {
	m_graphicsPipeline = _createGraphicsPipeline(
		device, graphicsLayout, renderPass, shaderPath, m_fragmentShader
	);
}

// Indirect Draw
GraphicsPipelineIndirectDraw::GraphicsPipelineIndirectDraw() noexcept
	: m_modelCount{ 0u }, m_counterBufferOffset{ 0u }, m_argumentBufferOffset{ 0u } {}

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

void GraphicsPipelineIndirectDraw::ConfigureGraphicsPipeline(
	const std::wstring& fragmentShader, std::uint32_t modelCount,
	std::uint32_t modelCountOffset, size_t counterIndex
) noexcept {
	m_modelCount = modelCount;

	m_argumentBufferOffset = sizeof(VkDrawIndexedIndirectCommand) * modelCountOffset;
	m_counterBufferOffset = sizeof(std::uint32_t) * 2u * counterIndex;
	m_fragmentShader = fragmentShader;
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineIndirectDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader
) const noexcept {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, renderPass, shaderPath, fragmentShader,
		L"VertexShaderIndirect.spv"
	);
}

// Individual Draw
GraphicsPipelineIndividualDraw::GraphicsPipelineIndividualDraw() noexcept
	: m_modelCount{ 0u }, m_modelOffset{ 0u } {}

void GraphicsPipelineIndividualDraw::ConfigureGraphicsPipeline(
	const std::wstring& fragmentShader, size_t modelCount, size_t modelOffset
) noexcept {
	m_fragmentShader = fragmentShader;
	m_modelCount = modelCount;
	m_modelOffset = modelOffset;
}

void GraphicsPipelineIndividualDraw::DrawModels(
	VkCommandBuffer graphicsCmdBuffer,
	const std::vector<VkDrawIndexedIndirectCommand>& drawArguments
) const noexcept {
	for (size_t index = 0u; index < m_modelCount; ++index) {
		const VkDrawIndexedIndirectCommand& modelArgs = drawArguments[m_modelOffset + index];

		vkCmdDrawIndexed(
			graphicsCmdBuffer, modelArgs.indexCount, modelArgs.instanceCount,
			modelArgs.firstIndex, modelArgs.vertexOffset, modelArgs.firstInstance
		);
	}
}

std::unique_ptr<VkPipelineObject> GraphicsPipelineIndividualDraw::_createGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader
) const noexcept {
	return CreateGraphicsPipelineVS(
		device, graphicsLayout, renderPass, shaderPath, fragmentShader,
		L"VertexShaderIndividual.spv"
	);
}
