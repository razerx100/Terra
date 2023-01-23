#ifndef RENDER_ENGINE_VERTEX_SHADER_HPP_
#define RENDER_ENGINE_VERTEX_SHADER_HPP_
#include <PipelineLayout.hpp>
#include <vector>
#include <GraphicsPipelineVertexShader.hpp>
#include <VertexManagerVertexShader.hpp>
#include <optional>

#include <RenderEngineBase.hpp>
#include <ComputePipelineIndirectDraw.hpp>

class RenderEngineVertexShader : public RenderEngineBase {
public:
	RenderEngineVertexShader(VkDevice device, QueueIndicesTG queueIndices);

	void AddGlobalVertices(
		VkDevice device, std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
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
	VertexManagerVertexShader m_vertexManager;
	QueueIndicesTG m_queueIndices;
};

class RenderEngineIndirectDraw final : public RenderEngineVertexShader {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<std::uint32_t> bufferCount;
		std::optional<QueueIndices3> queueIndices;
	};

public:
	RenderEngineIndirectDraw(Args& arguments);

	void ExecutePreRenderStage(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void ConstructPipelines(std::uint32_t frameCount) final;

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

	WaitSemaphoreData GetWaitSemaphores() const noexcept override;

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndirectDraw>;

private:
	ComputePipelineIndirectDraw m_computePipeline;
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;
};

class RenderEngineIndividualDraw final : public RenderEngineVertexShader {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<QueueIndicesTG> queueIndices;
	};

public:
	RenderEngineIndividualDraw(Args& arguments);

	void ExecutePreRenderStage(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void ConstructPipelines(std::uint32_t frameCount) final;

	void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
	) noexcept final;

private:
	WaitSemaphoreData GetWaitSemaphores() const noexcept override;

	void RecordModelArguments(const std::vector<std::shared_ptr<IModel>>& models) noexcept;

	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineIndividualDraw>;

private:
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	std::vector<VkDrawIndexedIndirectCommand> m_modelArguments;
};
#endif
