#ifndef RENDER_ENGINE_VS_HPP_
#define RENDER_ENGINE_VS_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineVSIndividualDeviceExtension : public RenderEngineDeviceExtension {};

class RenderEngineVSIndividual : public
	RenderEngineCommon
	<
		ModelManagerVSIndividual,
		MeshManagerVSIndividual,
		GraphicsPipelineVSIndividualDraw,
		RenderEngineVSIndividual
	>
{
	friend class RenderEngineCommon
		<
			ModelManagerVSIndividual,
			MeshManagerVSIndividual,
			GraphicsPipelineVSIndividualDraw,
			RenderEngineVSIndividual
		>;

public:
	RenderEngineVSIndividual(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader
	) override;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndividual GetModelManager(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager, std::uint32_t frameCount
	);
	[[nodiscard]]
	static ModelBuffers ConstructModelBuffers(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager, std::uint32_t frameCount
	);

	[[nodiscard]]
	VkSemaphore ExecutePipelineStages(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);

	[[nodiscard]]
	VkSemaphore GenericTransferStage(
		size_t frameIndex, std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);
	[[nodiscard]]
	VkSemaphore DrawingStage(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);

	void SetGraphicsDescriptorBufferLayout();
	void SetGraphicsDescriptors();

	void _updatePerFrame([[maybe_unused]]VkDeviceSize frameIndex) const noexcept {}

private:
	static constexpr std::uint32_t s_cameraBindingSlot = 1u;

public:
	RenderEngineVSIndividual(const RenderEngineVSIndividual&) = delete;
	RenderEngineVSIndividual& operator=(const RenderEngineVSIndividual&) = delete;

	RenderEngineVSIndividual(RenderEngineVSIndividual&& other) noexcept
		: RenderEngineCommon{ std::move(other) }
	{}
	RenderEngineVSIndividual& operator=(RenderEngineVSIndividual&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));

		return *this;
	}
};

class RenderEngineVSIndirectDeviceExtension : public RenderEngineDeviceExtension {};

class RenderEngineVSIndirect : public
	RenderEngineCommon
	<
		ModelManagerVSIndirect,
		MeshManagerVSIndirect,
		GraphicsPipelineVSIndirectDraw,
		RenderEngineVSIndirect
	>
{
	friend class RenderEngineCommon
		<
			ModelManagerVSIndirect,
			MeshManagerVSIndirect,
			GraphicsPipelineVSIndirectDraw,
			RenderEngineVSIndirect
		>;

	using ComputePipeline_t = ComputePipeline;

public:
	RenderEngineVSIndirect(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader
	) override;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

	void SetShaderPath(const std::wstring& shaderPath) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndirect GetModelManager(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager, std::uint32_t frameCount
	);
	[[nodiscard]]
	static ModelBuffers ConstructModelBuffers(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager, std::uint32_t frameCount
	);

	[[nodiscard]]
	VkSemaphore ExecutePipelineStages(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);

	[[nodiscard]]
	VkSemaphore GenericTransferStage(
		size_t frameIndex, std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);
	[[nodiscard]]
	VkSemaphore FrustumCullingStage(
		size_t frameIndex, std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);
	[[nodiscard]]
	VkSemaphore DrawingStage(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);

	void SetGraphicsDescriptorBufferLayout();
	void SetModelGraphicsDescriptors();

	void SetComputeDescriptorBufferLayout();
	void SetModelComputeDescriptors();

	void _updatePerFrame(VkDeviceSize frameIndex) const noexcept;

private:
	// Graphics
	static constexpr std::uint32_t s_cameraBindingSlot = 2u;

	// Compute
	static constexpr std::uint32_t s_computePipelineSetLayoutCount = 1u;
	static constexpr std::uint32_t s_computeShaderSetLayoutIndex   = 0u;

	static constexpr std::uint32_t s_modelBuffersComputeBindingSlot = 0u;
	static constexpr std::uint32_t s_cameraComputeBindingSlot       = 10u;

private:
	VkCommandQueue                     m_computeQueue;
	std::vector<VKSemaphore>           m_computeWait;
	std::vector<VkDescriptorBuffer>    m_computeDescriptorBuffers;
	PipelineManager<ComputePipeline_t> m_computePipelineManager;
	PipelineLayout                     m_computePipelineLayout;

public:
	RenderEngineVSIndirect(const RenderEngineVSIndirect&) = delete;
	RenderEngineVSIndirect& operator=(const RenderEngineVSIndirect&) = delete;

	RenderEngineVSIndirect(RenderEngineVSIndirect&& other) noexcept
		: RenderEngineCommon{ std::move(other) },
		m_computeQueue{ std::move(other.m_computeQueue) },
		m_computeWait{ std::move(other.m_computeWait) },
		m_computeDescriptorBuffers{ std::move(other.m_computeDescriptorBuffers) },
		m_computePipelineManager{ std::move(other.m_computePipelineManager) },
		m_computePipelineLayout{ std::move(other.m_computePipelineLayout) }
	{}
	RenderEngineVSIndirect& operator=(RenderEngineVSIndirect&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));
		m_computeQueue             = std::move(other.m_computeQueue);
		m_computeWait              = std::move(other.m_computeWait);
		m_computeDescriptorBuffers = std::move(other.m_computeDescriptorBuffers);
		m_computePipelineManager   = std::move(other.m_computePipelineManager);
		m_computePipelineLayout    = std::move(other.m_computePipelineLayout);

		return *this;
	}
};
#endif
