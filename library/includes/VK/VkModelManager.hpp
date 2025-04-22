#ifndef VK_MODEL_MANAGER_HPP_
#define VK_MODEL_MANAGER_HPP_
#include <VkResources.hpp>
#include <VkCommandQueue.hpp>
#include <VkExtensionManager.hpp>
#include <VkQueueFamilyManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VkStagingBufferManager.hpp>
#include <VkPipelineLayout.hpp>
#include <memory>
#include <ranges>
#include <algorithm>
#include <VkReusableBuffer.hpp>
#include <VkGraphicsPipelineVS.hpp>
#include <VkGraphicsPipelineMS.hpp>
#include <VkComputePipeline.hpp>
#include <TemporaryDataBuffer.hpp>
#include <VkMeshManager.hpp>
#include <VkPipelineManager.hpp>
#include <VkSharedBuffers.hpp>

#include <VkModelBundle.hpp>
#include <Shader.hpp>

template<class ModelBundleType>
class ModelManager
{
public:
	ModelManager() : m_modelBundles{} {}

	[[nodiscard]]
	std::optional<size_t> GetPipelineLocalIndex(
		std::uint32_t bundleIndex, std::uint32_t pipelineIndex
	) const noexcept {
		return m_modelBundles[bundleIndex].GetPipelineLocalIndex(pipelineIndex);
	}

protected:
	static void SetModelIndicesInBuffer(
		ModelBundle& modelBundle, const std::vector<std::uint32_t>& modelIndicesInBuffer
	) noexcept {
		std::vector<std::shared_ptr<Model>>& models = modelBundle.GetModels();

		// The number of indices and the number of models should be the same.
		const size_t modelCount = std::size(models);

		for (size_t index = 0u; index < modelCount; ++index)
			models[index]->SetModelIndexInBuffer(modelIndicesInBuffer[index]);
	}

protected:
	Callisto::ReusableVector<ModelBundleType> m_modelBundles;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_modelBundles{ std::move(other.m_modelBundles) }
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_modelBundles = std::move(other.m_modelBundles);

		return *this;
	}
};

template<class ModelBundleType>
class ModelManagerCommon : public ModelManager<ModelBundleType>
{
	friend class ModelManager<ModelBundleType>;

public:
	ModelManagerCommon() : ModelManager<ModelBundleType>{} {}

	// This should only be needed in the Indirect pipeline, as we need to allocate for that.
	void ReconfigureModels(
		[[maybe_unused]] std::uint32_t bundleIndex,
		[[maybe_unused]] std::uint32_t decreasedModelsPipelineIndex,
		[[maybe_unused]] std::uint32_t increasedModelsPipelineIndex
	) {}

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle,
		const std::vector<std::uint32_t>& modelIndicesInBuffer
	) {
		const size_t bundleIndex          = this->m_modelBundles.Add(ModelBundleType{});

		ModelBundleType& localModelBundle = this->m_modelBundles[bundleIndex];

		ModelManager<ModelBundleType>::SetModelIndicesInBuffer(
			*modelBundle, modelIndicesInBuffer
		);

		localModelBundle.SetModelBundle(std::move(modelBundle));

		localModelBundle.AddNewPipelinesFromBundle();

		return static_cast<std::uint32_t>(bundleIndex);
	}

	// The model bundle is needed to cleanup the buffer indices.
	[[nodiscard]]
	std::shared_ptr<ModelBundle> RemoveModelBundle(std::uint32_t bundleIndex) noexcept
	{
		const size_t bundleIndexST = bundleIndex;

		ModelBundleType& localModelBundle = this->m_modelBundles[bundleIndexST];

		std::shared_ptr<ModelBundle> modelBundle = localModelBundle.GetModelBundle();

		localModelBundle.CleanupData();

		this->m_modelBundles.RemoveElement(bundleIndexST);

		return modelBundle;
	}

public:
	ModelManagerCommon(const ModelManagerCommon&) = delete;
	ModelManagerCommon& operator=(const ModelManagerCommon&) = delete;

	ModelManagerCommon(ModelManagerCommon&& other) noexcept
		: ModelManager<ModelBundleType>{ std::move(other) }
	{}
	ModelManagerCommon& operator=(ModelManagerCommon&& other) noexcept
	{
		ModelManager<ModelBundleType>::operator=(std::move(other));

		return *this;
	}
};

class ModelManagerVSIndividual : public  ModelManagerCommon<ModelBundleVSIndividual>
{
	using Pipeline_t = GraphicsPipelineVSIndividualDraw;

public:
	ModelManagerVSIndividual() : ModelManagerCommon{} {}

	static void SetGraphicsConstantRange(PipelineLayout& layout) noexcept;

	void DrawPipeline(
		size_t modelBundleIndex, size_t pipelineLocalIndex, const VKCommandBuffer& graphicsBuffer,
		const MeshManagerVSIndividual& meshManager, VkPipelineLayout pipelineLayout
	) const noexcept;

public:
	ModelManagerVSIndividual(const ModelManagerVSIndividual&) = delete;
	ModelManagerVSIndividual& operator=(const ModelManagerVSIndividual&) = delete;

	ModelManagerVSIndividual(ModelManagerVSIndividual&& other) noexcept
		: ModelManagerCommon{ std::move(other) }
	{}
	ModelManagerVSIndividual& operator=(ModelManagerVSIndividual&& other) noexcept
	{
		ModelManagerCommon::operator=(std::move(other));

		return *this;
	}
};

class ModelManagerVSIndirect : public ModelManager<ModelBundleVSIndirect>
{
	using GraphicsPipeline_t = GraphicsPipelineVSIndirectDraw;
	using ComputePipeline_t  = ComputePipeline;

	struct ConstantData
	{
		std::uint32_t allocatedModelCount;
	};

	struct PerModelBundleData
	{
		std::uint32_t meshBundleIndex;
	};

public:
	ModelManagerVSIndirect(
		VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3,
		std::uint32_t frameCount
	);

	void ResetCounterBuffer(
		const VKCommandBuffer& computeCmdBuffer, size_t frameIndex
	) const noexcept;

	void SetCSPSOIndex(std::uint32_t psoIndex) noexcept { m_csPSOIndex = psoIndex; }

	static void SetComputeConstantRange(PipelineLayout& layout) noexcept;
	static void SetGraphicsConstantRange(PipelineLayout& layout) noexcept;

	// Will think about Adding a new model later.
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle,
		const std::vector<std::uint32_t>& modelIndicesInBuffer
	);

	// The model bundle is needed to cleanup the buffer indices.
	[[nodiscard]]
	std::shared_ptr<ModelBundle> RemoveModelBundle(std::uint32_t bundleIndex) noexcept;

	void SetDescriptorBufferLayoutVS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex
	) const noexcept;
	void SetDescriptorBuffersVS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex
	) const;

	void SetDescriptorBufferLayoutCS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
	) const noexcept;

	void SetDescriptorBuffersCS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
	) const;

	void ReconfigureModels(
		std::uint32_t bundleIndex, std::uint32_t decreasedModelsPipelineIndex,
		std::uint32_t increasedModelsPipelineIndex
	);

	void DrawPipeline(
		size_t frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
		const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndirect& meshManager,
		VkPipelineLayout pipelineLayout
	) const noexcept;

	void Dispatch(
		const VKCommandBuffer& computeBuffer,
		const PipelineManager<ComputePipeline_t>& pipelineManager
	) const noexcept;

	void UpdatePipelinePerFrame(
		VkDeviceSize frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
		const MeshManagerVSIndirect& meshManager, bool skipCulling
	) const noexcept;

private:
	void UpdateAllocatedModelCount() noexcept;
	void UpdateCounterResetValues();

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(ConstantData));
	}

private:
	std::vector<SharedBufferCPU>               m_argumentInputBuffers;
	std::vector<SharedBufferGPUWriteOnly>      m_argumentOutputBuffers;
	std::vector<SharedBufferGPUWriteOnly>      m_modelIndicesBuffers;
	SharedBufferCPU                            m_perPipelineBuffer;
	std::vector<SharedBufferGPUWriteOnly>      m_counterBuffers;
	Buffer                                     m_counterResetBuffer;
	MultiInstanceCPUBuffer<PerModelBundleData> m_perModelBundleBuffer;
	SharedBufferCPU                            m_perModelBuffer;
	QueueIndices3                              m_queueIndices3;
	std::uint32_t                              m_dispatchXCount;
	std::uint32_t                              m_allocatedModelCount;
	std::uint32_t                              m_csPSOIndex;

	// Vertex Shader ones
	// To read the model indices of the not culled models.
	static constexpr std::uint32_t s_modelIndicesVSBindingSlot   = 2u;

	// Compute Shader ones
	static constexpr std::uint32_t s_perModelBindingSlot         = 1u;
	static constexpr std::uint32_t s_argumentInputBindingSlot    = 2u;
	static constexpr std::uint32_t s_perPipelineBindingSlot      = 3u;
	static constexpr std::uint32_t s_argumenOutputBindingSlot    = 4u;
	static constexpr std::uint32_t s_counterBindingSlot          = 5u;
	static constexpr std::uint32_t s_perModelBundleBindingSlot   = 8u;
	// To write the model indices of the not culled models.
	static constexpr std::uint32_t s_modelIndicesVSCSBindingSlot = 9u;

	// Each Compute Thread Group should have 64 threads.
	static constexpr float THREADBLOCKSIZE = 64.f;

public:
	ModelManagerVSIndirect(const ModelManagerVSIndirect&) = delete;
	ModelManagerVSIndirect& operator=(const ModelManagerVSIndirect&) = delete;

	ModelManagerVSIndirect(ModelManagerVSIndirect&& other) noexcept
		: ModelManager{ std::move(other) },
		m_argumentInputBuffers{ std::move(other.m_argumentInputBuffers) },
		m_argumentOutputBuffers{ std::move(other.m_argumentOutputBuffers) },
		m_modelIndicesBuffers{ std::move(other.m_modelIndicesBuffers) },
		m_perPipelineBuffer{ std::move(other.m_perPipelineBuffer) },
		m_counterBuffers{ std::move(other.m_counterBuffers) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_perModelBundleBuffer{ std::move(other.m_perModelBundleBuffer) },
		m_perModelBuffer{ std::move(other.m_perModelBuffer) },
		m_queueIndices3{ other.m_queueIndices3 },
		m_dispatchXCount{ other.m_dispatchXCount },
		m_allocatedModelCount{ other.m_allocatedModelCount },
		m_csPSOIndex{ other.m_csPSOIndex }
	{}
	ModelManagerVSIndirect& operator=(ModelManagerVSIndirect&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_argumentInputBuffers   = std::move(other.m_argumentInputBuffers);
		m_argumentOutputBuffers  = std::move(other.m_argumentOutputBuffers);
		m_modelIndicesBuffers    = std::move(other.m_modelIndicesBuffers);
		m_perPipelineBuffer      = std::move(other.m_perPipelineBuffer);
		m_counterBuffers         = std::move(other.m_counterBuffers);
		m_counterResetBuffer     = std::move(other.m_counterResetBuffer);
		m_perModelBundleBuffer   = std::move(other.m_perModelBundleBuffer);
		m_perModelBuffer         = std::move(other.m_perModelBuffer);
		m_queueIndices3          = other.m_queueIndices3;
		m_dispatchXCount         = other.m_dispatchXCount;
		m_allocatedModelCount    = other.m_allocatedModelCount;
		m_csPSOIndex             = other.m_csPSOIndex;

		return *this;
	}
};

class ModelManagerMS : public ModelManagerCommon<ModelBundleMSIndividual>
{
	using Pipeline_t = GraphicsPipelineMS;

public:
	ModelManagerMS() : ModelManagerCommon{} {}

	static void SetGraphicsConstantRange(PipelineLayout& layout) noexcept;

	void DrawPipeline(
		size_t modelBundleIndex, size_t pipelineLocalIndex, const VKCommandBuffer& graphicsBuffer,
		const MeshManagerMS& meshManager, VkPipelineLayout pipelineLayout
	) const noexcept;

public:
	ModelManagerMS(const ModelManagerMS&) = delete;
	ModelManagerMS& operator=(const ModelManagerMS&) = delete;

	ModelManagerMS(ModelManagerMS&& other) noexcept
		: ModelManagerCommon{ std::move(other) }
	{}
	ModelManagerMS& operator=(ModelManagerMS&& other) noexcept
	{
		ModelManagerCommon::operator=(std::move(other));

		return *this;
	}
};
#endif
