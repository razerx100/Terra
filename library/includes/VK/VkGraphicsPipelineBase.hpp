#ifndef VK_GRAPHICS_PIPELINE_BASE_HPP_
#define VK_GRAPHICS_PIPELINE_BASE_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <VKPipelineObject.hpp>
#include <VkCommandQueue.hpp>
#include <VkPipelineLayout.hpp>
#include <ExternalPipeline.hpp>

template<typename Derived>
class GraphicsPipelineBase
{
public:
	GraphicsPipelineBase() : m_graphicsPipeline{}, m_graphicsExternalPipeline{} {}

	void Create(
		VkDevice device, VkPipelineLayout graphicsLayout,
		const std::wstring& shaderPath, const ExternalGraphicsPipeline& graphicsExtPipeline
	) {
		m_graphicsExternalPipeline = graphicsExtPipeline;

		m_graphicsPipeline = static_cast<Derived*>(this)->_createGraphicsPipeline(
			device, graphicsLayout, shaderPath, m_graphicsExternalPipeline
		);
	}

	void Recreate(
		VkDevice device, VkPipelineLayout graphicsLayout, const std::wstring& shaderPath
	) {
		m_graphicsPipeline = static_cast<Derived*>(this)->_createGraphicsPipeline(
			device, graphicsLayout, shaderPath, m_graphicsExternalPipeline
		);
	}

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
	{
		VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->Get());
	}

	[[nodiscard]]
	const ExternalGraphicsPipeline& GetExternalPipeline() const noexcept
	{
		return m_graphicsExternalPipeline;
	}

protected:
	std::unique_ptr<VkPipelineObject> m_graphicsPipeline;
	ExternalGraphicsPipeline          m_graphicsExternalPipeline;

	static constexpr ShaderBinaryType s_shaderBytecodeType = ShaderBinaryType::SPIRV;

public:
	GraphicsPipelineBase(const GraphicsPipelineBase&) = delete;
	GraphicsPipelineBase& operator=(const GraphicsPipelineBase&) = delete;

	GraphicsPipelineBase(GraphicsPipelineBase&& other) noexcept
		: m_graphicsPipeline{ std::move(other.m_graphicsPipeline) },
		m_graphicsExternalPipeline{ std::move(other.m_graphicsExternalPipeline) }
	{}
	GraphicsPipelineBase& operator=(GraphicsPipelineBase&& other) noexcept
	{
		m_graphicsPipeline         = std::move(other.m_graphicsPipeline);
		m_graphicsExternalPipeline = std::move(other.m_graphicsExternalPipeline);

		return *this;
	}
};

void ConfigurePipelineBuilder(
	GraphicsPipelineBuilder& builder, const ExternalGraphicsPipeline& graphicsExtPipeline
) noexcept;
#endif
