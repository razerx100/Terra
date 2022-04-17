#ifndef PIPELINE_OBJECT_GFX_HPP_
#define PIPELINE_OBJECT_GFX_HPP_
#include <vulkan/vulkan.hpp>

class PipelineObjectGFX {
public:
	PipelineObjectGFX(
		VkDevice device,
		VkPipelineLayout layout,
		VkRenderPass renderPass,
		const VkPipelineVertexInputStateCreateInfo* vertexInput,
		VkShaderModule vertexShader,
		VkShaderModule fragmentShader
	);
	~PipelineObjectGFX() noexcept;

	[[nodiscard]]
	VkPipeline GetPipelineObject() const noexcept;

private:
	VkDevice m_deviceRef;
	VkPipeline m_graphicsPipeline;
};
#endif
