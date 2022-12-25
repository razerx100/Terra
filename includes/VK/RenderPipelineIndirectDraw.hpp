#ifndef RENDER_PIPELINE_INDIRECT_DRAW_HPP_
#define RENDER_PIPELINE_INDIRECT_DRAW_HPP_
#include <vector>
#include <memory>
#include <VkResourceViews.hpp>
#include <VKPipelineObject.hpp>

#include <IModel.hpp>

struct CullingData {
	std::uint32_t commandCount;
	float padding;	// Next Vec2 starts at 8bytes offset
	DirectX::XMFLOAT2 xBounds;
	DirectX::XMFLOAT2 yBounds;
	DirectX::XMFLOAT2 zBounds;
};

class RenderPipelineIndirectDraw {
public:
	RenderPipelineIndirectDraw(
		VkDevice device, std::uint32_t bufferCount,
		std::vector<std::uint32_t> computeAndGraphicsQueueIndices
	) noexcept;

	void ConfigureGraphicsPipelineObject(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) noexcept;
	void RecordIndirectArguments(const std::vector<std::shared_ptr<IModel>>& models) noexcept;

	void CreateBuffers(VkDevice device) noexcept;
	void BindResourceToMemory(VkDevice device);
	void CopyData() noexcept;
	void RecordCopy(VkCommandBuffer copyBuffer) noexcept;
	void ReleaseUploadResources() noexcept;
	void AcquireOwnerShip(
		VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept;
	void ReleaseOwnership(
		VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept;

	void BindGraphicsPipeline(VkCommandBuffer graphicsCmdBuffer) const noexcept;
	void ResetCounterBuffer(VkCommandBuffer computeBuffer, VkDeviceSize frameIndex) noexcept;

	void DispatchCompute(VkCommandBuffer computeCmdBuffer) const noexcept;
	void DrawModels(VkCommandBuffer graphicsCmdBuffer, VkDeviceSize frameIndex) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> CreateGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader
	) const noexcept;

private:
	std::unique_ptr<VkPipelineObject> m_graphicsPSO;
	VkUploadableBufferResourceView m_commandBuffer;
	VkUploadableBufferResourceView m_culldataBuffer;
	VkResourceView m_counterResetBuffer;
	std::vector<VkResourceView> m_argumentBuffers;
	std::vector<VkResourceView> m_counterBuffers;
	std::uint32_t m_bufferCount;
	std::uint32_t m_modelCount;
	std::vector<VkDrawIndexedIndirectCommand> m_indirectCommands;
	std::vector<std::uint32_t> m_computeAndGraphicsQueueIndices;

	static constexpr float THREADBLOCKSIZE = 64.f;
	static constexpr DirectX::XMFLOAT2 XBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 YBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 ZBOUNDS = { 1.f, -1.f };
};
#endif
