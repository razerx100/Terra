#ifndef GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#define GRAPHICS_PIPELINE_VERTEX_SHADER_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineVertexShader : public GraphicsPipelineBase
{
public:
	GraphicsPipelineVertexShader() : GraphicsPipelineBase{} {}

	void CreateGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath
	) noexcept final;

protected:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineVS(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader,
		const std::wstring& vertexShader
	) const noexcept;
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
	GraphicsPipelineIndirectDraw() noexcept;

	void ConfigureGraphicsPipeline(
		const std::wstring& fragmentShader, std::uint32_t modelCount,
		std::uint32_t modelCountOffset, size_t counterIndex
	) noexcept;

	void DrawModels(
		VkCommandBuffer graphicsCmdBuffer, VkBuffer argumentBuffer, VkBuffer counterBuffer
	) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept final;

private:
	std::uint32_t m_modelCount;
	VkDeviceSize  m_counterBufferOffset;
	VkDeviceSize  m_argumentBufferOffset;

public:
	GraphicsPipelineIndirectDraw(const GraphicsPipelineIndirectDraw&) = delete;
	GraphicsPipelineIndirectDraw& operator=(const GraphicsPipelineIndirectDraw&) = delete;

	GraphicsPipelineIndirectDraw(GraphicsPipelineIndirectDraw&& other) noexcept
		: GraphicsPipelineVertexShader{ std::move(other) }, m_modelCount{ other.m_modelCount },
		m_counterBufferOffset{ other.m_counterBufferOffset },
		m_argumentBufferOffset{ other.m_argumentBufferOffset }
	{}
	GraphicsPipelineIndirectDraw& operator=(GraphicsPipelineIndirectDraw&& other) noexcept
	{
		GraphicsPipelineVertexShader::operator=(std::move(other));
		m_modelCount           = other.m_modelCount;
		m_counterBufferOffset  = other.m_counterBufferOffset;
		m_argumentBufferOffset = other.m_argumentBufferOffset;

		return *this;
	}
};

class GraphicsPipelineIndividualDraw : public GraphicsPipelineVertexShader
{
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
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept final;

private:
	size_t m_modelCount;
	size_t m_modelOffset;

public:
	GraphicsPipelineIndividualDraw(const GraphicsPipelineIndividualDraw&) = delete;
	GraphicsPipelineIndividualDraw& operator=(const GraphicsPipelineIndividualDraw&) = delete;

	GraphicsPipelineIndividualDraw(GraphicsPipelineIndividualDraw&& other) noexcept
		: GraphicsPipelineVertexShader{ std::move(other) }, m_modelCount{ other.m_modelCount },
		m_modelOffset{ other.m_modelOffset }
	{}
	GraphicsPipelineIndividualDraw& operator=(GraphicsPipelineIndividualDraw&& other) noexcept
	{
		GraphicsPipelineVertexShader::operator=(std::move(other));
		m_modelCount  = other.m_modelCount;
		m_modelOffset = other.m_modelOffset;

		return *this;
	}
};
#endif
