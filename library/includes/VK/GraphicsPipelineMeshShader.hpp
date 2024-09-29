#ifndef GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#define GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineMeshShader : public GraphicsPipelineBase<GraphicsPipelineMeshShader>
{
	friend class GraphicsPipelineBase<GraphicsPipelineMeshShader>;

public:
	GraphicsPipelineMeshShader(bool useTaskShader = true)
		: GraphicsPipelineBase{}, m_useTaskShader{ useTaskShader }
	{}

private:
	[[nodiscard]]
	static std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineMS(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		ShaderType binaryType, const std::wstring& shaderPath, const ShaderName& fragmentShader,
		const ShaderName& meshShader, const ShaderName& taskShader = {}
	);

	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const ShaderName& fragmentShader
	) const;

private:
	bool m_useTaskShader;

public:
	GraphicsPipelineMeshShader(const GraphicsPipelineMeshShader&) = delete;
	GraphicsPipelineMeshShader& operator=(const GraphicsPipelineMeshShader&) = delete;

	GraphicsPipelineMeshShader(GraphicsPipelineMeshShader&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }, m_useTaskShader{ other.m_useTaskShader }
	{}
	GraphicsPipelineMeshShader& operator=(GraphicsPipelineMeshShader&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));
		m_useTaskShader = other.m_useTaskShader;

		return *this;
	}
};
#endif
