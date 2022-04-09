#ifndef __GRAPHICS_PIPELINE_HPP__
#define __GRAPHICS_PIPELINE_HPP__
#include <IPipelineObject.hpp>

class PipelineObjectGFX : public IPipelineObject {
public:
	PipelineObjectGFX(
		VkDevice device,
		VkPipelineLayout layout,
		VkRenderPass renderPass,
		const VkPipelineVertexInputStateCreateInfo* vertexInput,
		VkShaderModule vertexShader,
		VkShaderModule fragmentShader
	);
	~PipelineObjectGFX() noexcept override;

	[[nodiscard]]
	VkPipeline GetPipelineObject() const noexcept override;

private:
	VkDevice m_deviceRef;
	VkPipeline m_graphicsPipeline;
};
#endif
