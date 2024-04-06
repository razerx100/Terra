#ifndef GRAPHICS_PIPELINE_BASE_HPP_
#define GRAPHICS_PIPELINE_BASE_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <VKPipelineObject.hpp>

class GraphicsPipelineBase
{
public:
	GraphicsPipelineBase() : m_graphicsPipeline{}, m_fragmentShader{} {}

	virtual void CreateGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath
	) noexcept = 0;

	void BindGraphicsPipeline(VkCommandBuffer graphicsCmdBuffer) const noexcept;

protected:
	std::unique_ptr<VkPipelineObject> m_graphicsPipeline;
	std::wstring                      m_fragmentShader;

public:
	GraphicsPipelineBase(const GraphicsPipelineBase&) = delete;
	GraphicsPipelineBase& operator=(const GraphicsPipelineBase&) = delete;

	GraphicsPipelineBase(GraphicsPipelineBase&& other) noexcept
		: m_graphicsPipeline{ std::move(other.m_graphicsPipeline) },
		m_fragmentShader{ std::move(other.m_fragmentShader) }
	{}
	GraphicsPipelineBase& operator=(GraphicsPipelineBase&& other) noexcept
	{
		m_graphicsPipeline = std::move(other.m_graphicsPipeline);
		m_fragmentShader   = std::move(other.m_fragmentShader);

		return *this;
	}
};
#endif
