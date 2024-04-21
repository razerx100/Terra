#ifndef GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#define GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineVertexShader : public GraphicsPipelineBase
{
public:
	GraphicsPipelineVertexShader() : GraphicsPipelineBase{} {}

	void Create(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) noexcept final;

protected:
	[[nodiscard]]
	static std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineVS(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader,
		const std::wstring& vertexShader
	) noexcept;
	[[nodiscard]]
	virtual std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept = 0;

public:
	GraphicsPipelineVertexShader(const GraphicsPipelineVertexShader&) = delete;
	GraphicsPipelineVertexShader& operator=(const GraphicsPipelineVertexShader&) = delete;

	GraphicsPipelineVertexShader(GraphicsPipelineVertexShader&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineVertexShader& operator=(GraphicsPipelineVertexShader&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};

class GraphicsPipelineIndirectDraw : public GraphicsPipelineVertexShader
{
public:
	GraphicsPipelineIndirectDraw() : GraphicsPipelineVertexShader{} {}

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept final;

public:
	GraphicsPipelineIndirectDraw(const GraphicsPipelineIndirectDraw&) = delete;
	GraphicsPipelineIndirectDraw& operator=(const GraphicsPipelineIndirectDraw&) = delete;

	GraphicsPipelineIndirectDraw(GraphicsPipelineIndirectDraw&& other) noexcept
		: GraphicsPipelineVertexShader{ std::move(other) }
	{}
	GraphicsPipelineIndirectDraw& operator=(GraphicsPipelineIndirectDraw&& other) noexcept
	{
		GraphicsPipelineVertexShader::operator=(std::move(other));

		return *this;
	}
};

class GraphicsPipelineIndividualDraw : public GraphicsPipelineVertexShader
{
public:
	GraphicsPipelineIndividualDraw() : GraphicsPipelineVertexShader{} {}

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept final;

public:
	GraphicsPipelineIndividualDraw(const GraphicsPipelineIndividualDraw&) = delete;
	GraphicsPipelineIndividualDraw& operator=(const GraphicsPipelineIndividualDraw&) = delete;

	GraphicsPipelineIndividualDraw(GraphicsPipelineIndividualDraw&& other) noexcept
		: GraphicsPipelineVertexShader{ std::move(other) }
	{}
	GraphicsPipelineIndividualDraw& operator=(GraphicsPipelineIndividualDraw&& other) noexcept
	{
		GraphicsPipelineVertexShader::operator=(std::move(other));

		return *this;
	}
};
#endif
