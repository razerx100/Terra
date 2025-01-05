#ifndef MODEL_MANAGER_HPP_
#define MODEL_MANAGER_HPP_
#include <VkResources.hpp>
#include <VkCommandQueue.hpp>
#include <VkExtensionManager.hpp>
#include <VkQueueFamilyManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <PipelineLayout.hpp>
#include <memory>
#include <ranges>
#include <algorithm>
#include <deque>
#include <mutex>
#include <ReusableVkBuffer.hpp>
#include <GraphicsPipelineVS.hpp>
#include <GraphicsPipelineMS.hpp>
#include <ComputePipeline.hpp>
#include <TemporaryDataBuffer.hpp>
#include <MeshManager.hpp>
#include <PipelineManager.hpp>

#include <VkModelBuffer.hpp>
#include <VkModelBundle.hpp>
#include <Shader.hpp>

template<class Derived, class ModelBundleType>
class ModelManager
{
public:
	ModelManager(MemoryManager* memoryManager) : m_memoryManager{ memoryManager }, m_modelBundles{} {}

	static void SetGraphicsConstantRange(PipelineLayout& layout) noexcept
	{
		Derived::_setGraphicsConstantRange(layout);
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, std::uint32_t psoIndex,
		ModelBuffers& modelBuffers, StagingBufferManager& stagingBufferMan,
		TemporaryDataBufferGPU& tempBuffer
	) {
		const std::vector<std::shared_ptr<Model>>& models = modelBundle->GetModels();

		if (!std::empty(models))
		{
			std::vector<std::shared_ptr<Model>> copyModels = models;
			std::vector<std::uint32_t> modelIndices        = modelBuffers.AddMultipleRU32(
				std::move(copyModels)
			);

			ModelBundleType modelBundleObj{};

			static_cast<Derived*>(this)->ConfigureModelBundle(
				modelBundleObj, std::move(modelIndices), std::move(modelBundle),
				stagingBufferMan, tempBuffer
			);

			modelBundleObj.SetPSOIndex(psoIndex);

			const std::uint32_t bundleID = modelBundleObj.GetID();

			AddModelBundle(std::move(modelBundleObj));

			return bundleID;
		}

		return std::numeric_limits<std::uint32_t>::max();
	}

	void RemoveModelBundle(std::uint32_t bundleID, ModelBuffers& modelBuffers) noexcept
	{
		auto result = GetModelBundle(bundleID);

		if (result != std::end(m_modelBundles))
		{
			const auto modelBundleIndex = static_cast<size_t>(
				std::distance(std::begin(m_modelBundles), result)
			);

			RemoveModelsFromModelBuffer(*result, modelBuffers);

			static_cast<Derived*>(this)->ConfigureModelBundleRemove(modelBundleIndex);

			m_modelBundles.erase(result);
		}
	}

	void ChangePSO(std::uint32_t bundleID, std::uint32_t psoIndex)
	{
		auto modelBundle = GetModelBundle(bundleID);

		if (modelBundle != std::end(m_modelBundles))
		{
			modelBundle->SetPSOIndex(psoIndex);

			ModelBundleType modelBundleObj = std::move(*modelBundle);

			m_modelBundles.erase(modelBundle);

			AddModelBundle(std::move(modelBundleObj));
		}
	}

protected:
	template<typename Pipeline>
	void BindPipeline(
		const ModelBundleType& modelBundle, const VKCommandBuffer& graphicsBuffer,
		const PipelineManager<Pipeline>& pipelineManager, size_t& previousPSOIndex
	) const noexcept {
		// PSO is more costly to bind, so the modelBundles are added in a way so they are sorted
		// by their PSO indices. And we only bind a new PSO, if the previous one was different.
		const size_t modelPSOIndex = modelBundle.GetPSOIndex();

		if (modelPSOIndex != previousPSOIndex)
		{
			pipelineManager.BindPipeline(modelPSOIndex, graphicsBuffer);

			previousPSOIndex = modelPSOIndex;
		}
	}

	[[nodiscard]]
	std::vector<ModelBundleType>::iterator GetModelBundle(std::uint32_t bundleID) noexcept
	{
		return std::ranges::find_if(
			m_modelBundles,
			[bundleID](const ModelBundleType& bundle) { return bundle.GetID() == bundleID; }
		);
	}

	virtual void ConfigureModelBundleRemove([[maybe_unused]] size_t modelBundleIndex) noexcept {}

private:
	void AddModelBundle(ModelBundleType&& modelBundle) noexcept
	{
		const std::uint32_t psoIndex = modelBundle.GetPSOIndex();

		// These will be sorted by their PSO indices.
		// Upper_bound returns the next bigger element.
		auto result = std::ranges::upper_bound(
			m_modelBundles, psoIndex, {},
			[](const ModelBundleType& modelBundle)
			{
				return modelBundle.GetPSOIndex();
			}
		);

		// If the result is the end it, that means there is no bigger index. So, then
		// insert at the back. Otherwise, insert at the end of the range of the same indices.
		// Insert works for the end iterator, so no need to emplace_back.
		m_modelBundles.insert(result, std::move(modelBundle));
	}

	static void RemoveModelsFromModelBuffer(
		const ModelBundleType& modelBundle, ModelBuffers& modelBuffers
	) {
		const std::vector<std::uint32_t>& modelIndices = modelBundle.GetModelIndices();

		for (std::uint32_t modelIndex : modelIndices)
			modelBuffers.Remove(modelIndex);
	}

protected:
	MemoryManager*               m_memoryManager;
	std::vector<ModelBundleType> m_modelBundles;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_memoryManager{ other.m_memoryManager },
		m_modelBundles{ std::move(other.m_modelBundles) }
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_memoryManager = other.m_memoryManager;
		m_modelBundles  = std::move(other.m_modelBundles);

		return *this;
	}
};

class ModelManagerVSIndividual : public ModelManager<ModelManagerVSIndividual, ModelBundleVSIndividual>
{
	friend class ModelManager<ModelManagerVSIndividual, ModelBundleVSIndividual>;

	using Pipeline_t = GraphicsPipelineVSIndividualDraw;

public:
	ModelManagerVSIndividual(MemoryManager* memoryManager);

	void Draw(
		const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndividual& meshManager,
		const PipelineManager<Pipeline_t>& pipelineManager
	) const noexcept;

private:
	static void _setGraphicsConstantRange(PipelineLayout& layout) noexcept;

	void ConfigureModelBundle(
		ModelBundleVSIndividual& modelBundleObj,
		std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle,
		StagingBufferManager& stagingBufferMan,
		TemporaryDataBufferGPU& tempBuffer
	) const noexcept;

public:
	ModelManagerVSIndividual(const ModelManagerVSIndividual&) = delete;
	ModelManagerVSIndividual& operator=(const ModelManagerVSIndividual&) = delete;

	ModelManagerVSIndividual(ModelManagerVSIndividual&& other) noexcept
		: ModelManager{ std::move(other) }
	{}
	ModelManagerVSIndividual& operator=(ModelManagerVSIndividual&& other) noexcept
	{
		ModelManager::operator=(std::move(other));

		return *this;
	}
};

class ModelManagerVSIndirect : public ModelManager<ModelManagerVSIndirect, ModelBundleVSIndirect>
{
	friend class ModelManager<ModelManagerVSIndirect, ModelBundleVSIndirect>;

	using GraphicsPipeline_t = GraphicsPipelineVSIndirectDraw;
	using ComputePipeline_t  = ComputePipeline;

	struct ConstantData
	{
		std::uint32_t modelCount;
	};

public:
	ModelManagerVSIndirect(
		VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3,
		std::uint32_t frameCount
	);

	void ResetCounterBuffer(const VKCommandBuffer& computeCmdBuffer, size_t frameIndex) const noexcept;

	static void SetComputeConstantRange(PipelineLayout& layout) noexcept;

	void CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept;

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

	void Draw(
		size_t frameIndex, const VKCommandBuffer& graphicsBuffer,
		const MeshManagerVSIndirect& meshManager,
		const PipelineManager<GraphicsPipeline_t>& pipelineManager
	) const noexcept;
	void Dispatch(
		const VKCommandBuffer& computeBuffer, const PipelineManager<ComputePipeline_t>& pipelineManager
	) const noexcept;

	void UpdatePerFrame(
		VkDeviceSize frameIndex, const MeshManagerVSIndirect& meshManager
	) const noexcept;

private:
	static void _setGraphicsConstantRange(PipelineLayout& layout) noexcept;

	void ConfigureModelBundle(
		ModelBundleVSIndirect& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle, StagingBufferManager& stagingBufferMan,
		TemporaryDataBufferGPU& tempBuffer
	);

	void ConfigureModelBundleRemove(size_t bundleIndex) noexcept override;

	void UpdateDispatchX() noexcept;
	void UpdateCounterResetValues();

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(ConstantData));
	}

private:
	std::vector<SharedBufferCPU>          m_argumentInputBuffers;
	std::vector<SharedBufferGPUWriteOnly> m_argumentOutputBuffers;
	std::vector<SharedBufferGPUWriteOnly> m_modelIndicesVSBuffers;
	SharedBufferCPU                       m_cullingDataBuffer;
	std::vector<SharedBufferGPUWriteOnly> m_counterBuffers;
	Buffer                                m_counterResetBuffer;
	MultiInstanceCPUBuffer<std::uint32_t> m_meshBundleIndexBuffer;
	SharedBufferGPU                       m_perModelDataCSBuffer;
	QueueIndices3                         m_queueIndices3;
	std::uint32_t                         m_dispatchXCount;
	std::uint32_t                         m_argumentCount;

	// These CS models will have the data to be uploaded and the dispatching will be done on the Manager.
	std::vector<ModelBundleCSIndirect>    m_modelBundlesCS;
	bool                                  m_oldBufferCopyNecessary;

	// Vertex Shader ones
	static constexpr std::uint32_t s_modelIndicesVSBindingSlot      = 1u;

	// Compute Shader ones
	static constexpr std::uint32_t s_perModelDataCSBindingSlot      = 1u;
	static constexpr std::uint32_t s_argumentInputBufferBindingSlot = 2u;
	static constexpr std::uint32_t s_cullingDataBufferBindingSlot   = 3u;
	static constexpr std::uint32_t s_argumenOutputBindingSlot       = 4u;
	static constexpr std::uint32_t s_counterBindingSlot             = 5u;
	static constexpr std::uint32_t s_meshBundleIndexBindingSlot     = 8u;
	// To write the model indices of the not culled models.
	static constexpr std::uint32_t s_modelIndicesVSCSBindingSlot    = 9u;

	// Each Compute Thread Group should have 64 threads.
	static constexpr float THREADBLOCKSIZE = 64.f;

public:
	ModelManagerVSIndirect(const ModelManagerVSIndirect&) = delete;
	ModelManagerVSIndirect& operator=(const ModelManagerVSIndirect&) = delete;

	ModelManagerVSIndirect(ModelManagerVSIndirect&& other) noexcept
		: ModelManager{ std::move(other) },
		m_argumentInputBuffers{ std::move(other.m_argumentInputBuffers) },
		m_argumentOutputBuffers{ std::move(other.m_argumentOutputBuffers) },
		m_modelIndicesVSBuffers{ std::move(other.m_modelIndicesVSBuffers) },
		m_cullingDataBuffer{ std::move(other.m_cullingDataBuffer) },
		m_counterBuffers{ std::move(other.m_counterBuffers) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_meshBundleIndexBuffer{ std::move(other.m_meshBundleIndexBuffer) },
		m_perModelDataCSBuffer{ std::move(other.m_perModelDataCSBuffer) },
		m_queueIndices3{ other.m_queueIndices3 },
		m_dispatchXCount{ other.m_dispatchXCount },
		m_argumentCount{ other.m_argumentCount },
		m_modelBundlesCS{ std::move(other.m_modelBundlesCS) },
		m_oldBufferCopyNecessary{ other.m_oldBufferCopyNecessary }
	{}
	ModelManagerVSIndirect& operator=(ModelManagerVSIndirect&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_argumentInputBuffers   = std::move(other.m_argumentInputBuffers);
		m_argumentOutputBuffers  = std::move(other.m_argumentOutputBuffers);
		m_modelIndicesVSBuffers  = std::move(other.m_modelIndicesVSBuffers);
		m_cullingDataBuffer      = std::move(other.m_cullingDataBuffer);
		m_counterBuffers         = std::move(other.m_counterBuffers);
		m_counterResetBuffer     = std::move(other.m_counterResetBuffer);
		m_meshBundleIndexBuffer  = std::move(other.m_meshBundleIndexBuffer);
		m_perModelDataCSBuffer   = std::move(other.m_perModelDataCSBuffer);
		m_queueIndices3          = other.m_queueIndices3;
		m_dispatchXCount         = other.m_dispatchXCount;
		m_argumentCount          = other.m_argumentCount;
		m_modelBundlesCS         = std::move(other.m_modelBundlesCS);
		m_oldBufferCopyNecessary = other.m_oldBufferCopyNecessary;

		return *this;
	}
};

class ModelManagerMS : public ModelManager<ModelManagerMS, ModelBundleMSIndividual>
{
	friend class ModelManager<ModelManagerMS, ModelBundleMSIndividual>;

	using Pipeline_t = GraphicsPipelineMS;

public:
	ModelManagerMS(MemoryManager* memoryManager);

	void Draw(
		const VKCommandBuffer& graphicsBuffer, const MeshManagerMS& meshManager,
		const PipelineManager<Pipeline_t>& pipelineManager
	) const noexcept;

private:
	static void _setGraphicsConstantRange(PipelineLayout& layout) noexcept;

	void ConfigureModelBundle(
		ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle, StagingBufferManager& stagingBufferMan,
		TemporaryDataBufferGPU& tempBuffer
	);

public:
	ModelManagerMS(const ModelManagerMS&) = delete;
	ModelManagerMS& operator=(const ModelManagerMS&) = delete;

	ModelManagerMS(ModelManagerMS&& other) noexcept
		: ModelManager{ std::move(other) }
	{}
	ModelManagerMS& operator=(ModelManagerMS&& other) noexcept
	{
		ModelManager::operator=(std::move(other));

		return *this;
	}
};
#endif
