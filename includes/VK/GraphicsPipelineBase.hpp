#ifndef GRAPHICS_PIPELINE_BASE_HPP_
#define GRAPHICS_PIPELINE_BASE_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <VKPipelineObject.hpp>

class GraphicsPipelineBase {
public:
	void CreateGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath
	) noexcept;

	void BindGraphicsPipeline(VkCommandBuffer graphicsCmdBuffer) const noexcept;

protected:
	[[nodiscard]]
	virtual std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept = 0;

protected:
	std::unique_ptr<VkPipelineObject> m_graphicsPipeline;
	std::wstring m_fragmentShader;
};
#endif
