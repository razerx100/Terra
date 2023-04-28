#ifndef GRAPHICS_PIPELINE_BASE_HPP_
#define GRAPHICS_PIPELINE_BASE_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <VKPipelineObject.hpp>

class GraphicsPipelineBase {
public:
	virtual void CreateGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath
	) noexcept = 0;

	void BindGraphicsPipeline(VkCommandBuffer graphicsCmdBuffer) const noexcept;

protected:
	std::unique_ptr<VkPipelineObject> m_graphicsPipeline;
	std::wstring m_fragmentShader;
};
#endif
