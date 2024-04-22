#ifndef RENDER_ENGINE_VERTEX_SHADER_HPP_
#define RENDER_ENGINE_VERTEX_SHADER_HPP_
#include <PipelineLayout.hpp>
#include <vector>
#include <GraphicsPipelineVertexShader.hpp>
#include <VertexManagerVertexShader.hpp>

#include <RenderEngineBase.hpp>
#include <ComputePipeline.hpp>

class RenderEngineVertexShader : public RenderEngineBase {
public:
	RenderEngineVertexShader(VkDevice device, QueueIndicesTG queueIndices);

	void AddGVerticesAndIndices(
		VkDevice device, std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) noexcept final;

	void BindResourcesToMemory(VkDevice device) final;
	void RecordCopy(VkCommandBuffer transferBuffer) noexcept final;
	void ReleaseUploadResources() noexcept final;
	void AcquireOwnerShipGraphics(VkCommandBuffer graphicsCmdBuffer) noexcept final;
	void ReleaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept final;

protected:
	void BindGraphicsBuffers(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex);

	virtual void _bindResourcesToMemory(VkDevice device);
	virtual void _recordCopy(VkCommandBuffer transferBuffer) noexcept;
	virtual void _releaseUploadResources() noexcept;
	virtual void _releaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept final;

private:
	//VertexManagerVertexShader m_vertexManager;
	QueueIndicesTG m_queueIndices;
};

class RenderEngineIndirectDraw final : public RenderEngineVertexShader
{
public:
	RenderEngineIndirectDraw(VkDevice device, std::uint32_t bufferCount, QueueIndices3 queueIndices);

	void ExecutePreRenderStage(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void ConstructPipelines() final;
	void UpdateModelBuffers(VkDeviceSize frameIndex) const noexcept final;

	void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
	) noexcept final;
	void CreateBuffers(VkDevice device) noexcept final;
	void CopyData() noexcept final;
	void AcquireOwnerShipCompute(VkCommandBuffer computeCmdBuffer) noexcept final;

private:
	void _bindResourcesToMemory(VkDevice device) override;
	void _recordCopy(VkCommandBuffer transferBuffer) noexcept override;
	void _releaseUploadResources() noexcept override;
	void _releaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept override;

	void ExecuteComputeStage(size_t frameIndex);

	[[nodiscard]]
	WaitSemaphoreData GetWaitSemaphores() const noexcept override;

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndirectDraw>;

private:
	//ComputePipelineIndirectDraw m_computePipeline;
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;
};

class RenderEngineIndividualDraw final : public RenderEngineVertexShader
{
public:
	RenderEngineIndividualDraw(VkDevice device, QueueIndicesTG queueIndices);

	void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void ConstructPipelines() final;
	void UpdateModelBuffers(VkDeviceSize frameIndex) const noexcept final;

	void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
	) noexcept final;

private:
	void RecordModelArguments(const std::vector<std::shared_ptr<IModel>>& models) noexcept;

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndividualDraw>;

private:
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	std::vector<VkDrawIndexedIndirectCommand> m_modelArguments;
};
#endif
