#ifndef RENDER_PIPELINE_HPP_
#define RENDER_PIPELINE_HPP_
#include <vector>
#include <memory>
#include <PipelineObjectGFX.hpp>
#include <PipelineLayout.hpp>

#include <IModel.hpp>

class RenderPipeline {
public:
	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;

	void AddGraphicsPipelineObject(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddGraphicsPipelineLayout(std::unique_ptr<PipelineLayout> layout) noexcept;

	void BindGraphicsPipeline(
		VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
	) const noexcept;

	void DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept;

private:
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	std::unique_ptr<PipelineObjectGFX> m_graphicsPSO;
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
};
#endif
