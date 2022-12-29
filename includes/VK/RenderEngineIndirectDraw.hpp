#ifndef RENDER_ENGINE_INDIRECT_DRAW_HPP_
#define RENDER_ENGINE_INDIRECT_DRAW_HPP_
#include <PipelineLayout.hpp>
#include <vector>
#include <GraphicsPipelineIndirectDraw.hpp>
#include <VertexManagerVertex.hpp>
#include <optional>

#include <RenderEngine.hpp>
#include <ComputePipelineIndirectDraw.hpp>

class RenderEngineIndirectDraw final : public RenderEngine {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<std::uint32_t> bufferCount;
		std::optional<QueueIndices3> queueIndices;
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

	void AddGlobalVertices(
		VkDevice device, std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	) noexcept override;
	void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
	) noexcept override;
	void CreateBuffers(VkDevice device) noexcept override;
	void BindResourcesToMemory(VkDevice device) override;
	void CopyData() noexcept override;
	void RecordCopy(VkCommandBuffer transferBuffer) noexcept override;
	void ReleaseUploadResources() noexcept override;
	void AcquireOwnerShipGraphics(VkCommandBuffer graphicsCmdBuffer) noexcept override;
	void AcquireOwnerShipCompute(VkCommandBuffer computeCmdBuffer) noexcept override;
	void ReleaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept override;

private:
	[[nodiscard]]
	std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept;

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndirectDraw>;

private:
	ComputePipelineIndirectDraw m_computePipeline;
	VertexManagerVertex m_vertexManager;
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	// Need to bind one pipeline first to bind descriptors
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;
	QueueIndicesTG m_queueIndices;
};
#endif
