#ifndef RENDER_ENGINE_MESH_SHADER_HPP_
#define RENDER_ENGINE_MESH_SHADER_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

/*
class RenderEngineMeshShader : public RenderEngineBase
{
public:
	RenderEngineMeshShader(VkDevice device, std::uint32_t bufferCount, QueueIndicesTG queueIndices);

	void AddMeshletModelSet(
		std::vector<MeshletModel>& meshletModels, const std::wstring& fragmentShader
	) noexcept override;
	void AddGVerticesAndPrimIndices(
		VkDevice device, std::vector<Vertex>&& gVertices,
		std::vector<std::uint32_t>&& gVerticesIndices, std::vector<std::uint32_t>&& gPrimIndices
	) noexcept override;
	void ConstructPipelines() final;
	void UpdateModelBuffers(VkDeviceSize frameIndex) const noexcept final;
	void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;

	void CreateBuffers(VkDevice device) noexcept final;
	void BindResourcesToMemory(VkDevice device) final;
	void RecordCopy(VkCommandBuffer transferBuffer) noexcept final;
	void ReleaseUploadResources() noexcept final;
	void AcquireOwnerShipGraphics(VkCommandBuffer graphicsCmdBuffer) noexcept final;
	void ReleaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept final;

private:
	using GraphicsPipeline = std::unique_ptr<GraphicsPipelineMeshShader>;

	[[nodiscard]]
	std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept final;

private:
	GraphicsPipeline m_graphicsPipeline0;
	std::vector<GraphicsPipeline> m_graphicsPipelines;

	//VertexManagerMeshShader m_vertexManager;
	//VkUploadableBufferResourceView m_meshletBuffer;
	std::vector<Meshlet> m_meshlets;

	QueueIndicesTG m_queueIndicesTG;
	std::uint32_t m_bufferCount;
};
*/
#endif
