#ifndef RENDER_ENGINE_BASE_HPP_
#define RENDER_ENGINE_BASE_HPP_
#include <memory>
#include <concepts>
#include <PipelineLayout.hpp>
#include <RenderEngine.hpp>
#include <RenderPassManager.hpp>

class RenderEngineBase : public RenderEngine {
public:
	void Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void ExecutePostRenderStage() final;

protected:
	void ExecutePreGraphicsStage(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex);
	void BindGraphicsDescriptorSets(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex);

	void ConstructGraphicsPipelineLayout(VkDevice device, std::uint32_t frameCount);

	[[nodiscard]]
	std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept;

	template<std::derived_from<GraphicsPipelineBase> Pipeline>
	void CreateGraphicsPipelines(
		VkDevice device, std::unique_ptr<Pipeline>& graphicsPipeline0,
		std::vector<std::unique_ptr<Pipeline>>& graphicsPipelines, VkRenderPass renderPass
	) const noexcept {
		VkPipelineLayout graphicsLayout = m_graphicsPipelineLayout->GetLayout();

		graphicsPipeline0->CreateGraphicsPipeline(
			device, graphicsLayout, renderPass, m_shaderPath
		);
		for (auto& graphicsPipeline : graphicsPipelines)
			graphicsPipeline->CreateGraphicsPipeline(
				device, graphicsLayout, renderPass, m_shaderPath
			);
	}

private:
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
};
#endif
