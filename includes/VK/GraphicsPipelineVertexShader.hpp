#ifndef GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#define GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineVertexShader : public GraphicsPipelineBase {
private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept final;
};

class GraphicsPipelineIndirectDraw : public GraphicsPipelineVertexShader {
public:
	GraphicsPipelineIndirectDraw() noexcept;

	void ConfigureGraphicsPipeline(
		const std::wstring& fragmentShader, std::uint32_t modelCount,
		std::uint32_t modelCountOffset, size_t counterIndex
	) noexcept;

	void DrawModels(
		VkCommandBuffer graphicsCmdBuffer, VkBuffer argumentBuffer, VkBuffer counterBuffer
	) const noexcept;

private:
	std::uint32_t m_modelCount;
	VkDeviceSize m_counterBufferOffset;
	VkDeviceSize m_argumentBufferOffset;
};

class GraphicsPipelineIndividualDraw : public GraphicsPipelineVertexShader {
public:
	GraphicsPipelineIndividualDraw() noexcept;

	void ConfigureGraphicsPipeline(
		const std::wstring& fragmentShader, size_t modelCount, size_t modelOffset
	) noexcept;

	void DrawModels(
		VkCommandBuffer graphicsCmdBuffer,
		const std::vector<VkDrawIndexedIndirectCommand>& drawArguments
	) const noexcept;

private:
	size_t m_modelCount;
	size_t m_modelOffset;
};
#endif
