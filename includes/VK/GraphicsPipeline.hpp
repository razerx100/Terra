#ifndef __GRAPHICS_PIPELINE_HPP__
#define __GRAPHICS_PIPELINE_HPP__
#include <vulkan/vulkan.hpp>

class GraphicsPipeline {
public:
	GraphicsPipeline(VkDevice device) noexcept;
	~GraphicsPipeline() noexcept;

	void CreatePipelineLayout();
	void CreateRenderPass();
	void CreateGraphicsPipeline();

private:
	VkDevice m_deviceRef;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;
	VkPipeline m_graphicsPipeline;
};
#endif
