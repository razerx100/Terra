#ifndef GRAPHICS_PIPELINE_INDIRECT_DRAW_HPP_
#define GRAPHICS_PIPELINE_INDIRECT_DRAW_HPP_
#include <memory>
#include <VKPipelineObject.hpp>

class GraphicsPipelineIndirectDraw {
public:
	GraphicsPipelineIndirectDraw() noexcept;

	void ConfigureGraphicsPipeline(
		const std::wstring& fragmentShader, std::uint32_t modelCount,
		std::uint32_t modelCountOffset
	) noexcept;
	void CreateGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath
	);

	void BindGraphicsPipeline(VkCommandBuffer graphicsCmdBuffer) const noexcept;
	void DrawModels(
		VkCommandBuffer graphicsCmdBuffer, VkBuffer argumentBuffer, VkBuffer counterBuffer
	) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept;

private:
	std::unique_ptr<VkPipelineObject> m_graphicsPipeline;
	std::uint32_t m_modelCount;
	VkDeviceSize m_counterBufferOffset;
	VkDeviceSize m_argumentBufferOffset;
	std::wstring m_fragmentShader;
};
#endif
