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

struct DepthStencilFormat
{
	VkFormat depthFormat;
	VkFormat stencilFormat;
};

template<typename Derived>
class GraphicsPipelineBase
{
public:
	GraphicsPipelineBase() : m_graphicsPipeline{}, m_fragmentShader{} {}

	void Create(
		VkDevice device, VkPipelineLayout graphicsLayout, VkFormat colourFormat,
		const DepthStencilFormat& depthStencilFormat,
		const std::wstring& shaderPath, const ShaderName& fragmentShader
	) {
		m_fragmentShader   = fragmentShader;

		m_graphicsPipeline = static_cast<Derived*>(this)->_createGraphicsPipeline(
			device, graphicsLayout, colourFormat, depthStencilFormat, shaderPath, m_fragmentShader
		);
	}

	void Recreate(
		VkDevice device, VkPipelineLayout graphicsLayout,
		VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat,
		const std::wstring& shaderPath
	) {
		m_graphicsPipeline = static_cast<Derived*>(this)->_createGraphicsPipeline(
			device, graphicsLayout, colourFormat, depthStencilFormat, shaderPath, m_fragmentShader
		);
	}

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
	{
		VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->Get());
	}

	[[nodiscard]]
	ShaderName GetShaderName() const noexcept { return m_fragmentShader; }

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
