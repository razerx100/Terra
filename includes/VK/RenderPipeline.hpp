#ifndef RENDER_PIPELINE_HPP_
#define RENDER_PIPELINE_HPP_
#include <vector>
#include <memory>
#include <VKPipelineObject.hpp>
#include <PipelineLayout.hpp>
#include <VkResourceViews.hpp>

#include <IModel.hpp>

struct CullingData {
	std::uint32_t commandCount;
	DirectX::XMFLOAT2 xBounds;
	float padding;
	DirectX::XMFLOAT2 yBounds;
	DirectX::XMFLOAT2 zBounds;
};

class RenderPipeline {
public:
	RenderPipeline(
		VkDevice device, std::uint32_t bufferCount,
		std::vector<std::uint32_t> computeAndGraphicsQueueIndices
	) noexcept;

	void AddGraphicsPipelineObject(std::unique_ptr<VkPipelineObject> graphicsPSO) noexcept;
	void AddGraphicsPipelineLayout(std::unique_ptr<PipelineLayout> graphicsLayout) noexcept;
	void AddComputePipelineObject(std::unique_ptr<VkPipelineObject> computePSO) noexcept;
	void AddComputePipelineLayout(std::unique_ptr<PipelineLayout> computeLayout) noexcept;

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

	void BindGraphicsPipeline(
		VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet graphicsDescriptorSet
	) const noexcept;
	void BindComputePipeline(
		VkCommandBuffer computeCmdBuffer, VkDescriptorSet computeDescriptorSet
	) const noexcept;
	void ResetCounterBuffer(VkCommandBuffer computeBuffer, VkDeviceSize frameIndex) noexcept;

	void DispatchCompute(VkCommandBuffer computeCmdBuffer) const noexcept;
	void DrawModels(VkCommandBuffer graphicsCmdBuffer, VkDeviceSize frameIndex) const noexcept;

private:
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	std::unique_ptr<VkPipelineObject> m_graphicsPSO;
	std::unique_ptr<PipelineLayout> m_computePipelineLayout;
	std::unique_ptr<VkPipelineObject> m_computePSO;
	VkUploadableBufferResourceView m_commandBuffer;
	VkUploadableBufferResourceView m_culldataBuffer;
	VkResourceView m_counterBuffer;
	std::vector<VkArgumentResourceView> m_argumentBuffers;
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
