#ifndef RENDER_ENGINE_BASE_HPP_
#define RENDER_ENGINE_BASE_HPP_
#include <RenderEngine.hpp>

/*
class RenderEngineBase : public RenderEngine {
public:
	void ExecutePreRenderStage(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) override;
	void Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void ExecutePostRenderStage() final;
	void ResizeViewportAndScissor(std::uint32_t width, std::uint32_t height) noexcept final;

	void AddGVerticesAndIndices(
		VkDevice device, std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) noexcept override;
	void RecordModelDataSet(
		const std::vector<std::shared_ptr<Model>>& models, const std::wstring& fragmentShader
	) noexcept override;
	void AddMeshletModelSet(
		std::vector<MeshletModel>& meshletModels, const std::wstring& fragmentShader
	) noexcept override;
	void AddGVerticesAndPrimIndices(
		VkDevice device, std::vector<Vertex>&& gVertices,
		std::vector<std::uint32_t>&& gVerticesIndices, std::vector<std::uint32_t>&& gPrimIndices
	) noexcept override;

protected:
	void ExecutePreGraphicsStage(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex);
	void BindGraphicsDescriptorSets(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex);

	void ConstructGraphicsPipelineLayout(VkDevice device);

	[[nodiscard]]
	virtual std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept = 0;

	using WaitSemaphoreData = std::pair<std::span<VkSemaphore>, std::span<VkPipelineStageFlags>>;

	[[nodiscard]]
	virtual WaitSemaphoreData GetWaitSemaphores() const noexcept;

	template<std::derived_from<GraphicsPipelineBase> Pipeline>
	void CreateGraphicsPipelines(
		VkDevice device, std::unique_ptr<Pipeline>& graphicsPipeline0,
		std::vector<std::unique_ptr<Pipeline>>& graphicsPipelines
	) const noexcept {
		VkPipelineLayout graphicsLayout = m_graphicsPipelineLayout->Get();

		graphicsPipeline0->CreateGraphicsPipeline(
			device, graphicsLayout, GetRenderPass(), m_shaderPath
		);
		for (auto& graphicsPipeline : graphicsPipelines)
			graphicsPipeline->CreateGraphicsPipeline(
				device, graphicsLayout, GetRenderPass(), m_shaderPath
			);
	}

private:
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	ViewportAndScissorManager m_viewportAndScissor;
};
*/
#endif
