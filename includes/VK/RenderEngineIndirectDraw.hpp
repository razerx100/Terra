#ifndef RENDER_ENGINE_INDIRECT_DRAW_HPP_
#define RENDER_ENGINE_INDIRECT_DRAW_HPP_
#include <PipelineLayout.hpp>
#include <vector>
#include <GraphicsPipelineIndirectDraw.hpp>
#include <optional>

#include <RenderEngine.hpp>
#include <ComputePipelineIndirectDraw.hpp>

class RenderEngineIndirectDraw final : public RenderEngine {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<std::uint32_t> bufferCount;
		std::optional<std::vector<std::uint32_t>> computeAndGraphicsQueueIndices;
	};

public:
	RenderEngineIndirectDraw(Args& arguments);

	void ExecutePreRenderStage(
		VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
	) override;
	void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) override;
	void Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) override;
	void ExecutePostRenderStage() override;
	void ConstructPipelines(std::uint32_t frameCount) override;

	void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
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

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndirectDraw>;

private:
	ComputePipelineIndirectDraw m_computePipeline;
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	// Need to bind one pipeline first to bind descriptors
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;
};
#endif
