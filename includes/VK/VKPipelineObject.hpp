#ifndef VK_PIPELINE_OBJECT_HPP_
#define VK_PIPELINE_OBJECT_HPP_
#include <vulkan/vulkan.hpp>
#include <VertexLayout.hpp>

class VkPipelineObject {
public:
	VkPipelineObject(VkDevice device) noexcept;
	~VkPipelineObject() noexcept;

	void CreateGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const VertexLayout& vertexInput, VkShaderModule vertexShader,
		VkShaderModule fragmentShader
	) noexcept;
	void CreateComputePipeline(
		VkDevice device, VkPipelineLayout computeLayout, VkShaderModule computeShader
	) noexcept;

	[[nodiscard]]
	VkPipeline GetPipeline() const noexcept;

private:
	[[nodiscard]]
	VkPipelineShaderStageCreateInfo GetShaderStageInfo(
		VkShaderModule shader, VkShaderStageFlagBits shaderType
	) const noexcept;

private:
	VkDevice m_deviceRef;
	VkPipeline m_pipeline;
};
#endif
