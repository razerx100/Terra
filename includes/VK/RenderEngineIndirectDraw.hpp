#ifndef RENDER_ENGINE_INDIRECT_DRAW_HPP_
#define RENDER_ENGINE_INDIRECT_DRAW_HPP_
#include <PipelineLayout.hpp>
#include <VKPipelineObject.hpp>

#include <RenderEngine.hpp>
#include <RenderPipelineIndirectDraw.hpp>

class RenderEngineIndirectDraw final : public RenderEngine {
public:
	void InitiatePipelines(
		VkDevice device, std::uint32_t bufferCount,
		std::vector<std::uint32_t> computeAndGraphicsQueueIndices = {}
	) noexcept override;
	void ExecutePreRenderStage(
		VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
	) override;
	void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) override;
	void Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) override;
	void ExecutePostRenderStage() override;
	void ConstructPipelines(std::uint32_t frameCount) override;

	void RecordModelData(
		const std::vector<std::shared_ptr<IModel>>& models
	) noexcept override;
	void CreateBuffers(VkDevice device) noexcept override;
	void BindResourcesToMemory(VkDevice device) override;
	void CopyData() noexcept override;
	void RecordCopy(VkCommandBuffer copyBuffer) noexcept override;
	void ReleaseUploadResources() noexcept override;
	void AcquireOwnerShip(
		VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept override;
	void ReleaseOwnership(
		VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept override;

private:
	[[nodiscard]]
	std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept;
	[[nodiscard]]
	std::unique_ptr<PipelineLayout> CreateComputePipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept;
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> CreateComputePipeline(
		VkDevice device, VkPipelineLayout computeLayout
	) const noexcept;

private:
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	std::unique_ptr<PipelineLayout> m_computePipelineLayout;
	std::unique_ptr<VkPipelineObject> m_computePSO;
	std::unique_ptr<RenderPipelineIndirectDraw> m_renderPipeline;
};
#endif
