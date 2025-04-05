#ifndef VK_MODEL_BUNDLE_HPP_
#define VK_MODEL_BUNDLE_HPP_
#include <utility>
#include <vector>
#include <memory>
#include <ranges>
#include <algorithm>
#include <Model.hpp>
#include <PipelineLayout.hpp>
#include <VkCommandQueue.hpp>
#include <VkMeshBundleMS.hpp>
#include <VkMeshBundleVS.hpp>
#include <PipelineManager.hpp>
#include <GraphicsPipelineVS.hpp>
#include <GraphicsPipelineMS.hpp>
#include <ReusableVector.hpp>

class PipelineModelsBase
{
public:
	PipelineModelsBase() : m_pipelineBundle{} {}

	[[nodiscard]]
	static VkDrawIndexedIndirectCommand GetDrawIndexedIndirectCommand(
		const MeshTemporaryDetailsVS& meshDetailsVS
	) noexcept;

	void SetPipelineModelBundle(std::shared_ptr<PipelineModelBundle> pipelineBundle) noexcept;

	void _cleanupData() noexcept { operator=(PipelineModelsBase{}); }

	[[nodiscard]]
	std::uint32_t GetPSOIndex() const noexcept { return m_pipelineBundle->GetPipelineIndex(); }

protected:
	std::shared_ptr<PipelineModelBundle> m_pipelineBundle;

public:
	PipelineModelsBase(const PipelineModelsBase&) = delete;
	PipelineModelsBase& operator=(const PipelineModelsBase&) = delete;

	PipelineModelsBase(PipelineModelsBase&& other) noexcept
		: m_pipelineBundle{ std::move(other.m_pipelineBundle) }
	{}
	PipelineModelsBase& operator=(PipelineModelsBase&& other) noexcept
	{
		m_pipelineBundle = std::move(other.m_pipelineBundle);

		return *this;
	}
};

class PipelineModelsVSIndividual : public PipelineModelsBase
{
public:
	PipelineModelsVSIndividual() : PipelineModelsBase{} {}

	void CleanupData() noexcept { _cleanupData(); }

	void Draw(
		const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
		const VkMeshBundleVS& meshBundle, const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(std::uint32_t));
	}

private:
	void DrawModel(
		const std::shared_ptr<Model>& model, VkCommandBuffer graphicsCmdBuffer,
		VkPipelineLayout pipelineLayout, const VkMeshBundleVS& meshBundle
	) const noexcept;

public:
	PipelineModelsVSIndividual(const PipelineModelsVSIndividual&) = delete;
	PipelineModelsVSIndividual& operator=(const PipelineModelsVSIndividual&) = delete;

	PipelineModelsVSIndividual(PipelineModelsVSIndividual&& other) noexcept
		: PipelineModelsBase{ std::move(other) }
	{}
	PipelineModelsVSIndividual& operator=(PipelineModelsVSIndividual&& other) noexcept
	{
		PipelineModelsBase::operator=(std::move(other));

		return *this;
	}
};

class PipelineModelsMSIndividual : public PipelineModelsBase
{
public:
	struct MeshDetails
	{
		std::uint32_t meshletCount;
		std::uint32_t meshletOffset;
		std::uint32_t indexOffset;
		std::uint32_t primOffset;
		std::uint32_t vertexOffset;
	};
	struct ModelDetails
	{
		MeshDetails   meshDetails;
		std::uint32_t modelBufferIndex;
	};

public:
	PipelineModelsMSIndividual() : PipelineModelsBase{} {}

	void CleanupData() noexcept { _cleanupData(); }

	void Draw(
		const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
		const VkMeshBundleMS& meshBundle, const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(ModelDetails));
	}

private:
	[[nodiscard]]
	static std::uint32_t DivRoundUp(std::uint32_t num, std::uint32_t den) noexcept
	{
		return (num + den - 1) / den;
	}

	void DrawModel(
		const std::shared_ptr<Model>& model, VkCommandBuffer graphicsCmdBuffer,
		VkPipelineLayout pipelineLayout, const VkMeshBundleMS& meshBundle
	) const noexcept;

private:
	static constexpr std::uint32_t s_taskInvocationCount = 32u;

	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkExtMeshShader
	};

public:
	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{
		return s_requiredExtensions;
	}

	PipelineModelsMSIndividual(const PipelineModelsMSIndividual&) = delete;
	PipelineModelsMSIndividual& operator=(const PipelineModelsMSIndividual&) = delete;

	PipelineModelsMSIndividual(PipelineModelsMSIndividual&& other) noexcept
		: PipelineModelsBase{ std::move(other) }
	{}
	PipelineModelsMSIndividual& operator=(PipelineModelsMSIndividual&& other) noexcept
	{
		PipelineModelsBase::operator=(std::move(other));

		return *this;
	}
};

class PipelineModelsCSIndirect : public PipelineModelsBase
{
	enum class ModelFlag : std::uint32_t
	{
		Visibility  = 1u,
		SkipCulling = 2u
	};

public:
	struct PerPipelineData
	{
		std::uint32_t modelCount;
		std::uint32_t modelOffset;
		std::uint32_t modelBundleIndex;
	};

	struct PerModelData
	{
		std::uint32_t pipelineIndex;
		std::uint32_t modelIndex;
		std::uint32_t modelFlags;
	};

public:
	PipelineModelsCSIndirect();

	void CleanupData() noexcept { operator=(PipelineModelsCSIndirect{}); }

	[[nodiscard]]
	size_t GetAddableModelCount() const noexcept;
	[[nodiscard]]
	size_t GetNewModelCount() const noexcept;

	void UpdateNonPerFrameData(
		std::uint32_t modelBundleIndex, const std::vector<std::shared_ptr<Model>>& models
	) noexcept;

	void AllocateBuffers(
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
	);

	void ResetCullingData() const noexcept;

	void Update(
		size_t frameIndex, const VkMeshBundleVS& meshBundle, bool skipCulling,
		const std::vector<std::shared_ptr<Model>>& models
	) const noexcept;

	void RelinquishMemory(
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
	) noexcept;

	[[nodiscard]]
	std::uint32_t GetModelCount() const noexcept
	{
		return static_cast<std::uint32_t>(std::size(m_pipelineBundle->GetModelIndicesInBundle()));
	}

	[[nodiscard]]
	static consteval size_t GetPerModelStride() noexcept
	{
		return sizeof(PerModelData);
	}

private:
	SharedBufferData              m_perPipelineSharedData;
	SharedBufferData              m_perModelSharedData;
	std::vector<SharedBufferData> m_argumentInputSharedData;

public:
	PipelineModelsCSIndirect(const PipelineModelsCSIndirect&) = delete;
	PipelineModelsCSIndirect& operator=(const PipelineModelsCSIndirect&) = delete;

	PipelineModelsCSIndirect(PipelineModelsCSIndirect&& other) noexcept
		: PipelineModelsBase{ std::move(other) },
		m_perPipelineSharedData{ other.m_perPipelineSharedData },
		m_perModelSharedData{ other.m_perModelSharedData },
		m_argumentInputSharedData{ std::move(other.m_argumentInputSharedData) }
	{}
	PipelineModelsCSIndirect& operator=(PipelineModelsCSIndirect&& other) noexcept
	{
		PipelineModelsBase::operator=(std::move(other));
		m_perPipelineSharedData     = other.m_perPipelineSharedData;
		m_perModelSharedData        = other.m_perModelSharedData;
		m_argumentInputSharedData   = std::move(other.m_argumentInputSharedData);

		return *this;
	}
};

class PipelineModelsVSIndirect
{
public:
	PipelineModelsVSIndirect();

	void SetModelCount(std::uint32_t count) noexcept { m_modelCount = count; }

	void AllocateBuffers(
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	);

	void CleanupData() noexcept { operator=(PipelineModelsVSIndirect{}); }

	void Draw(
		size_t frameIndex, const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
	) const noexcept;

	void RelinquishMemory(
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	) noexcept;

	[[nodiscard]]
	static VkDeviceSize GetCounterBufferSize() noexcept { return s_counterBufferSize; }

	[[nodiscard]]
	static constexpr std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(decltype(m_modelOffset)));
	}

private:
	std::vector<SharedBufferData> m_argumentOutputSharedData;
	std::vector<SharedBufferData> m_counterSharedData;
	std::vector<SharedBufferData> m_modelIndicesSharedData;
	std::uint32_t                 m_modelCount;
	std::uint32_t                 m_modelOffset;

	inline static VkDeviceSize s_counterBufferSize = static_cast<VkDeviceSize>(sizeof(std::uint32_t));

public:
	PipelineModelsVSIndirect(const PipelineModelsVSIndirect&) = delete;
	PipelineModelsVSIndirect& operator=(const PipelineModelsVSIndirect&) = delete;

	PipelineModelsVSIndirect(PipelineModelsVSIndirect&& other) noexcept
		: m_argumentOutputSharedData{ std::move(other.m_argumentOutputSharedData) },
		m_counterSharedData{ std::move(other.m_counterSharedData) },
		m_modelIndicesSharedData{ std::move(other.m_modelIndicesSharedData) },
		m_modelCount{ other.m_modelCount }, m_modelOffset{ other.m_modelOffset }
	{}
	PipelineModelsVSIndirect& operator=(PipelineModelsVSIndirect&& other) noexcept
	{
		m_argumentOutputSharedData = std::move(other.m_argumentOutputSharedData);
		m_counterSharedData        = std::move(other.m_counterSharedData);
		m_modelIndicesSharedData   = std::move(other.m_modelIndicesSharedData);
		m_modelCount               = other.m_modelCount;
		m_modelOffset              = other.m_modelOffset;

		return *this;
	}
};

template<typename Pipeline_t>
class ModelBundleBase
{
public:
	ModelBundleBase() : m_pipelines{}, m_modelBundle{} {}

	[[nodiscard]]
	std::optional<size_t> GetPipelineLocalIndex(std::uint32_t pipelineIndex) const noexcept
	{
		return FindPipeline(pipelineIndex);
	}

	void SetModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) noexcept
	{
		m_modelBundle = std::move(modelBundle);
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept
	{
		return m_modelBundle->GetMeshBundleIndex();
	}

	[[nodiscard]]
	std::uint32_t GetModelCount() const noexcept
	{
		return static_cast<std::uint32_t>(std::size(m_modelBundle->GetModels()));
	}

	[[nodiscard]]
	const std::shared_ptr<ModelBundle>& GetModelBundle() const noexcept { return m_modelBundle; }

protected:
	[[nodiscard]]
	std::optional<size_t> FindPipeline(std::uint32_t pipelineIndex) const noexcept
	{
		std::optional<size_t> index{};

		auto result = std::ranges::find(
			m_pipelines, pipelineIndex, [](const Pipeline_t& pipeline)
			{
				return pipeline.GetPSOIndex();
			}
		);

		if (result != std::end(m_pipelines))
			index = std::distance(std::begin(m_pipelines), result);

		return index;
	}

	void _cleanupData() noexcept { operator=(ModelBundleBase{}); }

	size_t _addPipeline(std::shared_ptr<PipelineModelBundle> pipelineBundle)
	{
		Pipeline_t pipeline{};

		pipeline.SetPipelineModelBundle(std::move(pipelineBundle));

		return m_pipelines.Add(std::move(pipeline));
	}

	void _removePipeline(size_t pipelineLocalIndex) noexcept
	{
		m_pipelines[pipelineLocalIndex].CleanupData();

		m_pipelines.RemoveElement(pipelineLocalIndex);
	}

protected:
	ReusableVector<Pipeline_t>   m_pipelines;
	std::shared_ptr<ModelBundle> m_modelBundle;

public:
	ModelBundleBase(const ModelBundleBase&) = delete;
	ModelBundleBase& operator=(const ModelBundleBase&) = delete;

	ModelBundleBase(ModelBundleBase&& other) noexcept
		: m_pipelines{ std::move(other.m_pipelines) },
		m_modelBundle{ std::move(other.m_modelBundle) }
	{}
	ModelBundleBase& operator=(ModelBundleBase&& other) noexcept
	{
		m_pipelines   = std::move(other.m_pipelines);
		m_modelBundle = std::move(other.m_modelBundle);

		return *this;
	}
};

template<typename Pipeline_t>
class ModelBundleCommon : public ModelBundleBase<Pipeline_t>
{
public:
	ModelBundleCommon() : ModelBundleBase<Pipeline_t>{} {}

	// Assuming any new pipelines will added at the back.
	void AddNewPipelinesFromBundle()
	{
		const std::vector<std::shared_ptr<PipelineModelBundle>>& pipelines
			= this->m_modelBundle->GetPipelineBundles();

		const size_t pipelinesInBundle    = std::size(pipelines);
		const size_t currentPipelineCount = std::size(this->m_pipelines);

		for (size_t index = currentPipelineCount; index < pipelinesInBundle; ++index)
			this->_addPipeline(pipelines[index]);
	}

	void RemovePipeline(size_t pipelineLocalIndex) noexcept
	{
		this->_removePipeline(pipelineLocalIndex);
	}

	void CleanupData() noexcept { this->_cleanupData(); }

public:
	ModelBundleCommon(const ModelBundleCommon&) = delete;
	ModelBundleCommon& operator=(const ModelBundleCommon&) = delete;

	ModelBundleCommon(ModelBundleCommon&& other) noexcept
		: ModelBundleBase<Pipeline_t>{ std::move(other) }
	{}
	ModelBundleCommon& operator=(ModelBundleCommon&& other) noexcept
	{
		ModelBundleBase<Pipeline_t>::operator=(std::move(other));

		return *this;
	}
};

class ModelBundleVSIndividual : public ModelBundleCommon<PipelineModelsVSIndividual>
{
	using GraphicsPipeline_t = GraphicsPipelineVSIndividualDraw;

public:
	ModelBundleVSIndividual() : ModelBundleCommon{} {}

	void DrawPipeline(
		size_t pipelineLocalIndex, const VKCommandBuffer& graphicsBuffer,
		VkPipelineLayout pipelineLayout, const VkMeshBundleVS& meshBundle
	) const noexcept;

public:
	ModelBundleVSIndividual(const ModelBundleVSIndividual&) = delete;
	ModelBundleVSIndividual& operator=(const ModelBundleVSIndividual&) = delete;

	ModelBundleVSIndividual(ModelBundleVSIndividual&& other) noexcept
		: ModelBundleCommon{ std::move(other) }
	{}
	ModelBundleVSIndividual& operator=(ModelBundleVSIndividual&& other) noexcept
	{
		ModelBundleCommon::operator=(std::move(other));

		return *this;
	}
};

class ModelBundleMSIndividual : public ModelBundleCommon<PipelineModelsMSIndividual>
{
	using GraphicsPipeline_t = GraphicsPipelineMS;

public:
	ModelBundleMSIndividual() : ModelBundleCommon{} {}

	void DrawPipeline(
		size_t pipelineLocalIndex, const VKCommandBuffer& graphicsBuffer,
		VkPipelineLayout pipelineLayout, const VkMeshBundleMS& meshBundle
	) const noexcept;

private:
	static void SetMeshBundleConstants(
		VkCommandBuffer graphicsBuffer, VkPipelineLayout pipelineLayout, const VkMeshBundleMS& meshBundle
	) noexcept;

public:
	ModelBundleMSIndividual(const ModelBundleMSIndividual&) = delete;
	ModelBundleMSIndividual& operator=(const ModelBundleMSIndividual&) = delete;

	ModelBundleMSIndividual(ModelBundleMSIndividual&& other) noexcept
		: ModelBundleCommon{ std::move(other) }
	{}
	ModelBundleMSIndividual& operator=(ModelBundleMSIndividual&& other) noexcept
	{
		ModelBundleCommon::operator=(std::move(other));

		return *this;
	}
};

class ModelBundleVSIndirect : public ModelBundleBase<PipelineModelsCSIndirect>
{
	using GraphicsPipeline_t = GraphicsPipelineVSIndirectDraw;

public:
	ModelBundleVSIndirect() : ModelBundleBase{}, m_vsPipelines{} {}

	// Assuming any new pipelines will added at the back.
	void AddNewPipelinesFromBundle(
		std::uint32_t modelBundleIndex, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	);

	void RemovePipeline(
		size_t pipelineLocalIndex, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	) noexcept;

	void CleanupData(
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	) noexcept;

	void ReconfigureModels(
		std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
		std::uint32_t increasedModelsPipelineIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	);

	void UpdatePipeline(
		size_t pipelineLacalIndex, size_t frameIndex, const VkMeshBundleVS& meshBundle,
		bool skipCulling
	) const noexcept;

	void DrawPipeline(
		size_t pipelineLocalIndex, size_t frameIndex, const VKCommandBuffer& graphicsBuffer,
		VkPipelineLayout pipelineLayout, const VkMeshBundleVS& meshBundle
	) const noexcept;

	void SetupPipelineBuffers(
		std::uint32_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	);

private:
	void ResizePreviousPipelines(
		size_t addableStartIndex, size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	);

	void RecreateFollowingPipelines(
		size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
		SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	);

	[[nodiscard]]
	size_t FindAddableStartIndex(size_t pipelineLocalIndex, size_t modelCount) const noexcept;

	[[nodiscard]]
	size_t GetLocalPipelineIndex(std::uint32_t pipelineIndex);

private:
	std::vector<PipelineModelsVSIndirect> m_vsPipelines;

public:
	ModelBundleVSIndirect(const ModelBundleVSIndirect&) = delete;
	ModelBundleVSIndirect& operator=(const ModelBundleVSIndirect&) = delete;

	ModelBundleVSIndirect(ModelBundleVSIndirect&& other) noexcept
		: ModelBundleBase{ std::move(other) },
		m_vsPipelines{ std::move(other.m_vsPipelines) }
	{}
	ModelBundleVSIndirect& operator=(ModelBundleVSIndirect&& other) noexcept
	{
		ModelBundleBase::operator=(std::move(other));
		m_vsPipelines            = std::move(other.m_vsPipelines);

		return *this;
	}
};
#endif
