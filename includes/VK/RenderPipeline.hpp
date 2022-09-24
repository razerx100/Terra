#ifndef RENDER_PIPELINE_HPP_
#define RENDER_PIPELINE_HPP_
#include <vector>
#include <memory>
#include <PipelineObjectGFX.hpp>
#include <PipelineLayout.hpp>
#include <VkResourceViews.hpp>

#include <IModel.hpp>

struct ModelConstantBuffer {
	UVInfo uvInfo;
	DirectX::XMMATRIX modelMatrix;
	std::uint32_t textureIndex;
	float padding[3];
};

class RenderPipeline {
public:
	RenderPipeline(VkDevice device) noexcept;

	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void AddGraphicsPipelineObject(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddGraphicsPipelineLayout(std::unique_ptr<PipelineLayout> layout) noexcept;

	void CreateBuffers(VkDevice device, std::uint32_t bufferCount) noexcept;
	void UpdateModelData(size_t frameIndex) const noexcept;
	void BindResourceToMemory(VkDevice device);
	void BindGraphicsPipeline(
		VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
	) const noexcept;

	void DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept;

private:
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	std::unique_ptr<PipelineObjectGFX> m_graphicsPSO;
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
	VkResourceView m_modelBuffers;
};
#endif
