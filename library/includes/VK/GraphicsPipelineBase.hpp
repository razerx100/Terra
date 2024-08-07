#ifndef GRAPHICS_PIPELINE_BASE_HPP_
#define GRAPHICS_PIPELINE_BASE_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <VKPipelineObject.hpp>
#include <VkCommandQueue.hpp>
#include <VKRenderPass.hpp>
#include <PipelineLayout.hpp>
#include <Shader.hpp>

class GraphicsPipelineBase
{
public:
	GraphicsPipelineBase() : m_graphicsPipeline{}, m_fragmentShader{} {}

	virtual void Create(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const ShaderName& fragmentShader
	) = 0;

	void Create(
		VkDevice device, const PipelineLayout& graphicsLayout, const VKRenderPass& renderPass,
		const std::wstring& shaderPath, const ShaderName& fragmentShader
	) {
		Create(device, graphicsLayout.Get(), renderPass.Get(), shaderPath, fragmentShader);
	}

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

	[[nodiscard]]
	ShaderName GetFragmentShader() const noexcept { return m_fragmentShader; }

protected:
	std::unique_ptr<VkPipelineObject> m_graphicsPipeline;
	ShaderName                        m_fragmentShader;

	static constexpr ShaderType s_shaderBytecodeType = ShaderType::SPIRV;

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
