#ifndef VK_GRAPHICS_PIPELINE_MS_HPP_
#define VK_GRAPHICS_PIPELINE_MS_HPP_
#include <VkGraphicsPipelineBase.hpp>

class GraphicsPipelineMS : public GraphicsPipelineBase<GraphicsPipelineMS>
{
	friend class GraphicsPipelineBase<GraphicsPipelineMS>;

public:
	GraphicsPipelineMS() : GraphicsPipelineBase{} {}

private:
	[[nodiscard]]
	static std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineMS(
		VkDevice device, VkPipelineLayout graphicsLayout,
		ShaderBinaryType binaryType, const std::wstring& shaderPath,
		const ExternalGraphicsPipeline& graphicsExtPipeline, const ShaderName& taskShader
	);

	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout,
		const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
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
