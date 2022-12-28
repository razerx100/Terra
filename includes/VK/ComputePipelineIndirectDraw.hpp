#ifndef COMPUTE_PIPELINE_INDIRECT_DRAW_HPP_
#define COMPUTE_PIPELINE_INDIRECT_DRAW_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <PipelineLayout.hpp>
#include <VKPipelineObject.hpp>
#include <VkResourceViews.hpp>

#include <IModel.hpp>
#include <DirectXMath.h>

class ComputePipelineIndirectDraw {
public:
	ComputePipelineIndirectDraw(
		VkDevice device, std::uint32_t bufferCount,
		std::vector<std::uint32_t> computeAndGraphicsQueueIndices
	) noexcept;

	void CreateComputePipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) noexcept;
	void CreateComputePipeline(VkDevice device, const std::wstring& shaderPath) noexcept;
	void CreateBuffers(VkDevice device) noexcept;
	void BindResourceToMemory(VkDevice device);
	void RecordIndirectArguments(const std::vector<std::shared_ptr<IModel>>& models) noexcept;
	void CopyData() noexcept;
	void RecordCopy(VkCommandBuffer copyBuffer) noexcept;
	void ReleaseUploadResources() noexcept;
	void AcquireOwnerShip(
		VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept;
	void ReleaseOwnership(
		VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept;

	void BindComputePipeline(
		VkCommandBuffer computeCmdBuffer, size_t frameIndex
	) const noexcept;
	void ResetCounterBuffer(VkCommandBuffer computeCmdBuffer, VkDeviceSize frameIndex) noexcept;
	void DispatchCompute(VkCommandBuffer computeCmdBuffer) const noexcept;

	[[nodiscard]]
	std::uint32_t GetCurrentModelCount() const noexcept;
	[[nodiscard]]
	size_t GetCounterCount() const noexcept;
	[[nodiscard]]
	VkBuffer GetArgumentBuffer(size_t frameIndex) const noexcept;
	[[nodiscard]]
	VkBuffer GetCounterBuffer(size_t frameIndex) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<PipelineLayout> _createComputePipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept;
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createComputePipeline(
		VkDevice device, VkPipelineLayout computeLayout, const std::wstring& shaderPath
	) const noexcept;

private:
	std::unique_ptr<PipelineLayout> m_computePipelineLayout;
	std::unique_ptr<VkPipelineObject> m_computePipeline;
	VkUploadableBufferResourceView m_commandBuffer;
	VkUploadableBufferResourceView m_culldataBuffer;
	VkResourceView m_counterResetBuffer;
	std::vector<VkResourceView> m_argumentBuffers;
	std::vector<VkUploadableBufferResourceView> m_counterBuffers;
	std::vector<VkDrawIndexedIndirectCommand> m_indirectCommands;
	std::vector<std::uint32_t> m_computeAndGraphicsQueueIndices;
	std::vector<std::uint32_t> m_modelCountOffsets;
	std::uint32_t m_bufferCount;
	std::uint32_t m_modelCount;

	static constexpr float THREADBLOCKSIZE = 64.f;
	static constexpr DirectX::XMFLOAT2 XBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 YBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 ZBOUNDS = { 1.f, -1.f };
	static constexpr VkDeviceSize COUNTERBUFFERSTRIDE =
		static_cast<VkDeviceSize>(sizeof(std::uint32_t) * 2u);

private:
	struct CullingData {
		std::uint32_t commandCount;
		std::uint32_t modelTypes;	// Next Vec2 starts at 8bytes offset
		DirectX::XMFLOAT2 xBounds;
		DirectX::XMFLOAT2 yBounds;
		DirectX::XMFLOAT2 zBounds;
	};
};
#endif
