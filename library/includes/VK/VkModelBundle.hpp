#ifndef VK_MODEL_BUNDLE_HPP_
#define VK_MODEL_BUNDLE_HPP_
#include <utility>
#include <vector>
#include <memory>
#include <Model.hpp>
#include <PipelineLayout.hpp>
#include <VkCommandQueue.hpp>
#include <VkMeshBundleMS.hpp>
#include <VkMeshBundleVS.hpp>

class ModelBundleBase
{
public:
	ModelBundleBase() : m_psoIndex{ 0u } {}

	void SetPSOIndex(std::uint32_t index) noexcept { m_psoIndex = index; }

	[[nodiscard]]
	std::uint32_t GetPSOIndex() const noexcept { return m_psoIndex; }

	[[nodiscard]]
	static VkDrawIndexedIndirectCommand GetDrawIndexedIndirectCommand(
		const MeshTemporaryDetailsVS& meshDetailsVS
	) noexcept;

protected:
	std::uint32_t m_psoIndex;

public:
	ModelBundleBase(const ModelBundleBase&) = delete;
	ModelBundleBase& operator=(const ModelBundleBase&) = delete;

	ModelBundleBase(ModelBundleBase&& other) noexcept
		: m_psoIndex{ other.m_psoIndex }
	{}
	ModelBundleBase& operator=(ModelBundleBase&& other) noexcept
	{
		m_psoIndex  = other.m_psoIndex;

		return *this;
	}
};

class ModelBundleVSIndividual : public ModelBundleBase
{
public:
	ModelBundleVSIndividual() : ModelBundleBase{}, m_modelBufferIndices{}, m_modelBundle{} {}

	void SetModelBundle(
		std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
	) noexcept;
	void Draw(
		const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
		const VkMeshBundleVS& meshBundle
	) const noexcept;

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(std::uint32_t));
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelIndices() const noexcept { return m_modelBufferIndices; }

	[[nodiscard]]
	std::uint32_t GetID() const noexcept
	{
		if (!std::empty(m_modelBufferIndices))
			return m_modelBufferIndices.front();
		else
			return std::numeric_limits<std::uint32_t>::max();
	}

private:
	std::vector<std::uint32_t>   m_modelBufferIndices;
	std::shared_ptr<ModelBundle> m_modelBundle;

public:
	ModelBundleVSIndividual(const ModelBundleVSIndividual&) = delete;
	ModelBundleVSIndividual& operator=(const ModelBundleVSIndividual&) = delete;

	ModelBundleVSIndividual(ModelBundleVSIndividual&& other) noexcept
		: ModelBundleBase{ std::move(other) },
		m_modelBufferIndices{ std::move(other.m_modelBufferIndices) },
		m_modelBundle{ std::move(other.m_modelBundle) }
	{}
	ModelBundleVSIndividual& operator=(ModelBundleVSIndividual&& other) noexcept
	{
		ModelBundleBase::operator=(std::move(other));
		m_modelBufferIndices     = std::move(other.m_modelBufferIndices);
		m_modelBundle            = std::move(other.m_modelBundle);

		return *this;
	}
};

class ModelBundleMSIndividual : public ModelBundleBase
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
	ModelBundleMSIndividual() : ModelBundleBase{}, m_modelBundle{}, m_modelBufferIndices{} {}

	void SetModelBundle(
		std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
	) noexcept;

	void Draw(
		const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
		const VkMeshBundleMS& meshBundle
	) const noexcept;

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelIndices() const noexcept { return m_modelBufferIndices; }
	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }

	[[nodiscard]]
	std::uint32_t GetID() const noexcept
	{
		if (!std::empty(m_modelBufferIndices))
			return m_modelBufferIndices.front();
		else
			return std::numeric_limits<std::uint32_t>::max();
	}
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

private:
	std::shared_ptr<ModelBundle> m_modelBundle;
	std::vector<std::uint32_t>   m_modelBufferIndices;

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

	ModelBundleMSIndividual(const ModelBundleMSIndividual&) = delete;
	ModelBundleMSIndividual& operator=(const ModelBundleMSIndividual&) = delete;

	ModelBundleMSIndividual(ModelBundleMSIndividual&& other) noexcept
		: ModelBundleBase{ std::move(other) },
		m_modelBundle{ std::move(other.m_modelBundle) },
		m_modelBufferIndices{ std::move(other.m_modelBufferIndices) }
	{}
	ModelBundleMSIndividual& operator=(ModelBundleMSIndividual&& other) noexcept
	{
		ModelBundleBase::operator=(std::move(other));
		m_modelBundle            = std::move(other.m_modelBundle);
		m_modelBufferIndices     = std::move(other.m_modelBufferIndices);

		return *this;
	}
};

class ModelBundleCSIndirect
{
public:
	struct CullingData
	{
		std::uint32_t commandCount;
		std::uint32_t commandOffset;// Next Vec2 starts at 8bytes offset
	};

	struct PerModelData
	{
		std::uint32_t bundleIndex;
		std::uint32_t modelIndex;
	};

public:
	ModelBundleCSIndirect();

	void SetModelBundle(std::shared_ptr<ModelBundle> bundle) noexcept;
	void CreateBuffers(
		StagingBufferManager& stagingBufferMan,
		std::vector<SharedBufferCPU>& argumentInputSharedBuffer,
		SharedBufferCPU& cullingSharedBuffer, SharedBufferGPU& perModelDataCSBuffer,
		const std::vector<std::uint32_t>& modelIndices, TemporaryDataBufferGPU& tempBuffer
	);

	void SetID(std::uint32_t bundleID) noexcept { m_bundleID = bundleID; }
	void ResetCullingData() const noexcept;

	void Update(size_t bufferIndex, const VkMeshBundleVS& meshBundle) const noexcept;

	[[nodiscard]]
	std::uint32_t GetID() const noexcept { return m_bundleID; }
	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }

	[[nodiscard]]
	// Must be called after the buffers have been created.
	std::uint32_t GetModelBundleIndex() const noexcept
	{
		return static_cast<std::uint32_t>(m_cullingSharedData.offset / sizeof(CullingData));
	}

	[[nodiscard]]
	const std::vector<SharedBufferData>& GetArgumentInputSharedData() const noexcept
	{
		return m_argumentInputSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetCullingSharedData() const noexcept { return m_cullingSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetPerModelDataCSSharedData() const noexcept
	{
		return m_perModelDataCSSharedData;
	}

private:
	SharedBufferData              m_cullingSharedData;
	SharedBufferData              m_perModelDataCSSharedData;
	std::vector<SharedBufferData> m_argumentInputSharedData;
	std::shared_ptr<ModelBundle>  m_modelBundle;
	std::uint32_t                 m_bundleID;

public:
	ModelBundleCSIndirect(const ModelBundleCSIndirect&) = delete;
	ModelBundleCSIndirect& operator=(const ModelBundleCSIndirect&) = delete;

	ModelBundleCSIndirect(ModelBundleCSIndirect&& other) noexcept
		: m_cullingSharedData{ other.m_cullingSharedData },
		m_perModelDataCSSharedData{ other.m_perModelDataCSSharedData },
		m_argumentInputSharedData{ std::move(other.m_argumentInputSharedData) },
		m_modelBundle{ std::move(other.m_modelBundle) },
		m_bundleID{ other.m_bundleID }
	{}
	ModelBundleCSIndirect& operator=(ModelBundleCSIndirect&& other) noexcept
	{
		m_cullingSharedData        = other.m_cullingSharedData;
		m_perModelDataCSSharedData = other.m_perModelDataCSSharedData;
		m_argumentInputSharedData  = std::move(other.m_argumentInputSharedData);
		m_modelBundle              = std::move(other.m_modelBundle);
		m_bundleID                 = other.m_bundleID;

		return *this;
	}
};

class ModelBundleVSIndirect : public ModelBundleBase
{
public:
	ModelBundleVSIndirect();

	void SetModelBundle(
		std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
	) noexcept;

	void CreateBuffers(
		std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
		std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
	);
	void Draw(
		size_t frameIndex, const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
	) const noexcept;

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelIndices() const noexcept { return m_modelIndices; }

	[[nodiscard]]
	std::uint32_t GetID() const noexcept
	{
		if (!std::empty(m_modelIndices))
			return m_modelIndices.front();
		else
			return std::numeric_limits<std::uint32_t>::max();
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }
	[[nodiscard]]
	std::uint32_t GetModelCount() const noexcept
	{
		return static_cast<std::uint32_t>(std::size(m_modelIndices));
	}
	[[nodiscard]]
	std::uint32_t GetModelOffset() const noexcept { return m_modelOffset; }
	[[nodiscard]]
	const std::vector<SharedBufferData>& GetArgumentOutputSharedData() const noexcept
	{
		return m_argumentOutputSharedData;
	}
	[[nodiscard]]
	const std::vector<SharedBufferData>& GetCounterSharedData() const noexcept
	{
		return m_counterSharedData;
	}
	[[nodiscard]]
	const std::vector<SharedBufferData>& GetModelIndicesSharedData() const noexcept
	{
		return m_modelIndicesSharedData;
	}

	[[nodiscard]]
	static VkDeviceSize GetCounterBufferSize() noexcept { return s_counterBufferSize; }

	[[nodiscard]]
	static constexpr std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(m_modelOffset));
	}

private:
	std::uint32_t                 m_modelOffset;
	std::shared_ptr<ModelBundle>  m_modelBundle;
	std::vector<SharedBufferData> m_argumentOutputSharedData;
	std::vector<SharedBufferData> m_counterSharedData;
	std::vector<SharedBufferData> m_modelIndicesSharedData;

	// I am gonna use the DrawIndex in the Vertex shader and the thread Index in the Compute shader
	// to index into this buffer and that will give us the actual model index.
	// Should replace this with a better alternative one day.
	std::vector<std::uint32_t>    m_modelIndices;

	inline static VkDeviceSize s_counterBufferSize = static_cast<VkDeviceSize>(sizeof(std::uint32_t));

public:
	ModelBundleVSIndirect(const ModelBundleVSIndirect&) = delete;
	ModelBundleVSIndirect& operator=(const ModelBundleVSIndirect&) = delete;

	ModelBundleVSIndirect(ModelBundleVSIndirect&& other) noexcept
		: ModelBundleBase{ std::move(other) }, m_modelOffset{ other.m_modelOffset },
		m_modelBundle{ std::move(other.m_modelBundle) },
		m_argumentOutputSharedData{ std::move(other.m_argumentOutputSharedData) },
		m_counterSharedData{ std::move(other.m_counterSharedData) },
		m_modelIndicesSharedData{ std::move(other.m_modelIndicesSharedData) },
		m_modelIndices{ std::move(other.m_modelIndices) }
	{}
	ModelBundleVSIndirect& operator=(ModelBundleVSIndirect&& other) noexcept
	{
		ModelBundleBase::operator=(std::move(other));
		m_modelOffset              = other.m_modelOffset;
		m_modelBundle              = std::move(other.m_modelBundle);
		m_argumentOutputSharedData = std::move(other.m_argumentOutputSharedData);
		m_counterSharedData        = std::move(other.m_counterSharedData);
		m_modelIndicesSharedData   = std::move(other.m_modelIndicesSharedData);
		m_modelIndices             = std::move(other.m_modelIndices);

		return *this;
	}
};
#endif
