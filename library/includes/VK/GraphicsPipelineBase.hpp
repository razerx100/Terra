#ifndef GRAPHICS_PIPELINE_BASE_HPP_
#define GRAPHICS_PIPELINE_BASE_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <VKPipelineObject.hpp>
#include <VkCommandQueue.hpp>

class GraphicsPipelineBase
{
public:
	GraphicsPipelineBase() : m_graphicsPipeline{}, m_fragmentShader{} {}

	virtual void Create(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) noexcept = 0;

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

	[[nodiscard]]
	std::wstring GetFragmentShader() const noexcept { return m_fragmentShader; }

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
