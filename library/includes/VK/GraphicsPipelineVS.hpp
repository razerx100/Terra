#ifndef GRAPHICS_PIPELINE_VS_HPP_
#define GRAPHICS_PIPELINE_VS_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineVSIndirectDraw : public GraphicsPipelineBase<GraphicsPipelineVSIndirectDraw>
{
	friend class GraphicsPipelineBase<GraphicsPipelineVSIndirectDraw>;

public:
	GraphicsPipelineVSIndirectDraw() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const ShaderName& fragmentShader
	) const;

public:
	GraphicsPipelineVSIndirectDraw(const GraphicsPipelineVSIndirectDraw&) = delete;
	GraphicsPipelineVSIndirectDraw& operator=(const GraphicsPipelineVSIndirectDraw&) = delete;

	GraphicsPipelineVSIndirectDraw(GraphicsPipelineVSIndirectDraw&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineVSIndirectDraw& operator=(GraphicsPipelineVSIndirectDraw&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};

class GraphicsPipelineVSIndividualDraw : public GraphicsPipelineBase<GraphicsPipelineVSIndividualDraw>
{
	friend class GraphicsPipelineBase<GraphicsPipelineVSIndividualDraw>;

public:
	GraphicsPipelineVSIndividualDraw() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const ShaderName& fragmentShader
	) const;

public:
	GraphicsPipelineVSIndividualDraw(const GraphicsPipelineVSIndividualDraw&) = delete;
	GraphicsPipelineVSIndividualDraw& operator=(const GraphicsPipelineVSIndividualDraw&) = delete;

	GraphicsPipelineVSIndividualDraw(GraphicsPipelineVSIndividualDraw&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineVSIndividualDraw& operator=(GraphicsPipelineVSIndividualDraw&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};
#endif