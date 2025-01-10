#ifndef GRAPHICS_PIPELINE_MS_HPP_
#define GRAPHICS_PIPELINE_MS_HPP_
#include <GraphicsPipelineBase.hpp>

class GraphicsPipelineMS : public GraphicsPipelineBase<GraphicsPipelineMS>
{
	friend class GraphicsPipelineBase<GraphicsPipelineMS>;

public:
	GraphicsPipelineMS() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	static std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineMS(
		VkDevice device, VkPipelineLayout graphicsLayout,
		VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat,
		ShaderType binaryType, const std::wstring& shaderPath, const ShaderName& fragmentShader,
		const ShaderName& meshShader, const ShaderName& taskShader
	);

	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout,
		VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat,
		const std::wstring& shaderPath, const ShaderName& fragmentShader
	) const;

public:
	GraphicsPipelineMS(const GraphicsPipelineMS&) = delete;
	GraphicsPipelineMS& operator=(const GraphicsPipelineMS&) = delete;

	GraphicsPipelineMS(GraphicsPipelineMS&& other) noexcept
		: GraphicsPipelineBase{ std::move(other) }
	{}
	GraphicsPipelineMS& operator=(GraphicsPipelineMS&& other) noexcept
	{
		GraphicsPipelineBase::operator=(std::move(other));

		return *this;
	}
};
#endif
