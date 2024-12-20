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
#include <GraphicsPipelineVertexShader.hpp>
#include <GraphicsPipelineMeshShader.hpp>
#include <ComputePipeline.hpp>
#include <TemporaryDataBuffer.hpp>

#include <MeshManagerVertexShader.hpp>
#include <MeshManagerMeshShader.hpp>
#include <CommonBuffers.hpp>
#include <Model.hpp>
#include <Shader.hpp>

class ModelBundleBase
{
public:
	ModelBundleBase() : m_psoIndex{ 0u } {}

	void SetPSOIndex(std::uint32_t index) noexcept { m_psoIndex = index; }

	[[nodiscard]]
	std::uint32_t GetPSOIndex() const noexcept { return m_psoIndex; }

	[[nodiscard]]
	static VkDrawIndexedIndirectCommand GetDrawIndexedIndirectCommand(
		const MeshDetails& meshDetails
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
		const MeshManagerVertexShader& meshBundle
	) const noexcept;

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(std::uint32_t));
	}

	[[nodiscard]]
	std::uint32_t GetMeshBundleIndex() const noexcept { return m_modelBundle->GetMeshBundleIndex(); }
	[[nodiscard]]
	const std::vector<std::uint32_t>& GetIndices() const noexcept { return m_modelBufferIndices; }

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
	struct ModelDetails
	{
		std::uint32_t meshletCount;
		std::uint32_t meshletOffset;
		std::uint32_t modelBufferIndex;
	};

public:
	ModelBundleMSIndividual() : ModelBundleBase{}, m_modelBundle{}, m_modelBufferIndices{} {}

	void SetModelBundle(
		std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
	) noexcept;

	void Draw(
		const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
		const MeshManagerMeshShader& meshBundle
	) const noexcept;

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetIndices() const noexcept { return m_modelBufferIndices; }
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

	void Update(size_t bufferIndex, const MeshManagerVertexShader& meshBundle) const noexcept;

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

class ModelBuffers : public ReusableVkBuffer<ModelBuffers, std::shared_ptr<Model>>
{
	friend class ReusableVkBuffer<ModelBuffers, std::shared_ptr<Model>>;

public:
	ModelBuffers(
		VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount,
		const std::vector<std::uint32_t>& modelBufferQueueIndices
	) : ReusableVkBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_fragmentModelBuffers{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_modelBuffersInstanceSize{ 0u }, m_modelBuffersFragmentInstanceSize{ 0u },
		m_bufferInstanceCount{ frameCount }, m_modelBuffersQueueIndices{ modelBufferQueueIndices }
	{}

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot,
		size_t setLayoutIndex
	) const;
	void SetFragmentDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot,
		size_t setLayoutIndex
	) const;

	void Update(VkDeviceSize bufferIndex) const noexcept;

	[[nodiscard]]
	std::uint32_t GetInstanceCount() const noexcept { return m_bufferInstanceCount; }

private:
	struct ModelVertexData
	{
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMFLOAT3 modelOffset;
		// GLSL vec3 is actually vec4, so the materialIndex must be grabbed from the z component.
		std::uint32_t     materialIndex;
		std::uint32_t     meshIndex;
		float             modelScale;
		// This struct is 16 bytes aligned thanks to the Matrix. So, putting the correct
		// amount of padding. Just to make it more obvious though, as it would have been
		// put anyway.
		std::uint32_t     padding[2];
	};

	struct ModelFragmentData
	{
		UVInfo        diffuseTexUVInfo;
		UVInfo        specularTexUVInfo;
		std::uint32_t diffuseTexIndex;
		std::uint32_t specularTexIndex;
		float         padding[2]; // Needs to be 16bytes aligned.
	};

private:
	[[nodiscard]]
	static consteval size_t GetVertexStride() noexcept { return sizeof(ModelVertexData); }
	[[nodiscard]]
	static consteval size_t GetFragmentStride() noexcept { return sizeof(ModelFragmentData); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t modelCount);

private:
	Buffer                     m_fragmentModelBuffers;
	VkDeviceSize               m_modelBuffersInstanceSize;
	VkDeviceSize               m_modelBuffersFragmentInstanceSize;
	std::uint32_t              m_bufferInstanceCount;
	std::vector<std::uint32_t> m_modelBuffersQueueIndices;

public:
	ModelBuffers(const ModelBuffers&) = delete;
	ModelBuffers& operator=(const ModelBuffers&) = delete;

	ModelBuffers(ModelBuffers&& other) noexcept
		: ReusableVkBuffer{ std::move(other) },
		m_fragmentModelBuffers{ std::move(other.m_fragmentModelBuffers) },
		m_modelBuffersInstanceSize{ other.m_modelBuffersInstanceSize },
		m_modelBuffersFragmentInstanceSize{ other.m_modelBuffersFragmentInstanceSize },
		m_bufferInstanceCount{ other.m_bufferInstanceCount },
		m_modelBuffersQueueIndices{ std::move(other.m_modelBuffersQueueIndices) }
	{}
	ModelBuffers& operator=(ModelBuffers&& other) noexcept
	{
		ReusableVkBuffer::operator=(std::move(other));
		m_fragmentModelBuffers             = std::move(other.m_fragmentModelBuffers);
		m_modelBuffersInstanceSize         = other.m_modelBuffersInstanceSize;
		m_modelBuffersFragmentInstanceSize = other.m_modelBuffersFragmentInstanceSize;
		m_bufferInstanceCount              = other.m_bufferInstanceCount;
		m_modelBuffersQueueIndices         = std::move(other.m_modelBuffersQueueIndices);

		return *this;
	}
};

template<
	class Derived,
	class Pipeline,
	class MeshManager,
	class ModelBundleType
>
class ModelManager
{
public:
	ModelManager(
		VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices, std::uint32_t frameCount
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_graphicsPipelineLayout{ VK_NULL_HANDLE }, m_renderPass{ VK_NULL_HANDLE }, m_shaderPath{},
		m_modelBuffers{ Derived::ConstructModelBuffers(device, memoryManager, frameCount, queueIndices) },
		m_graphicsPipelines{}, m_meshBundles{}, m_modelBundles{}, m_tempCopyNecessary{ true }
	{}

	static void SetGraphicsConstantRange(PipelineLayout& layout) noexcept
	{
		Derived::_setGraphicsConstantRange(layout);
	}

	void SetGraphicsPipelineLayout(VkPipelineLayout layout) noexcept
	{
		m_graphicsPipelineLayout = layout;
	}

	void UpdatePerFrame(VkDeviceSize frameIndex) const noexcept
	{
		m_modelBuffers.Update(frameIndex);

		static_cast<Derived const*>(this)->_updatePerFrame(frameIndex);
	}

	void SetRenderPass(VkRenderPass renderPass) noexcept
	{
		m_renderPass = renderPass;
	}

	void SetShaderPath(std::wstring shaderPath)
	{
		m_shaderPath = std::move(shaderPath);

		static_cast<Derived*>(this)->ShaderPathSet();
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader,
		TemporaryDataBufferGPU& tempBuffer
	) {
		const auto& models      = modelBundle->GetModels();
		const size_t modelCount = std::size(models);

		if (modelCount)
		{
			std::vector<std::shared_ptr<Model>> tempModelBundle{ modelCount, nullptr };

			for (size_t index = 0u; index < modelCount; ++index)
				tempModelBundle[index] = std::static_pointer_cast<Model>(models[index]);

			const std::vector<size_t> modelIndices
				= m_modelBuffers.AddMultiple(std::move(tempModelBundle));

			auto dvThis = static_cast<Derived*>(this);

			ModelBundleType modelBundleObj{};

			{
				std::vector<std::uint32_t> modelIndicesU32(std::size(modelIndices), 0u);
				for (size_t index = 0u; index < std::size(modelIndices); ++index)
					modelIndicesU32[index] = static_cast<std::uint32_t>(modelIndices[index]);

				dvThis->ConfigureModelBundle(
					modelBundleObj, std::move(modelIndicesU32), std::move(modelBundle), tempBuffer
				);
			}

			const std::uint32_t psoIndex = GetPSOIndex(fragmentShader);

			modelBundleObj.SetPSOIndex(psoIndex);

			const auto bundleID = modelBundleObj.GetID();

			AddModelBundle(std::move(modelBundleObj));

			m_tempCopyNecessary = true;

			return bundleID;
		}

		return std::numeric_limits<std::uint32_t>::max();
	}

	void RemoveModelBundle(std::uint32_t bundleID) noexcept
	{
		auto result = GetModelBundle(bundleID);

		if (result != std::end(m_modelBundles))
		{
			const auto modelBundleIndex = static_cast<size_t>(
				std::distance(std::begin(m_modelBundles), result)
			);

			static_cast<Derived*>(this)->ConfigureModelRemove(modelBundleIndex);

			m_modelBundles.erase(result);
		}
	}

	void AddPSO(const ShaderName& fragmentShader)
	{
		GetPSOIndex(fragmentShader);
	}

	void ChangePSO(std::uint32_t bundleID, const ShaderName& fragmentShader)
	{
		auto modelBundle = GetModelBundle(bundleID);

		if (modelBundle != std::end(m_modelBundles))
		{
			const std::uint32_t psoIndex = GetPSOIndex(fragmentShader);

			modelBundle->SetPSOIndex(psoIndex);

			ModelBundleType modelBundleObj = std::move(*modelBundle);

			m_modelBundles.erase(modelBundle);

			AddModelBundle(std::move(modelBundleObj));
		}
	}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		TemporaryDataBufferGPU& tempBuffer
	) {
		MeshManager meshManager{};

		static_cast<Derived*>(this)->ConfigureMeshBundle(
			std::move(meshBundle), stagingBufferMan, meshManager, tempBuffer
		);

		auto meshIndex = m_meshBundles.Add(std::move(meshManager));

		m_tempCopyNecessary = true;

		return static_cast<std::uint32_t>(meshIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
	{
		static_cast<Derived*>(this)->ConfigureRemoveMesh(bundleIndex);

		// It is okay to use the non-clear function based RemoveElement, as I will be
		// moving the Buffers out as SharedBuffer.
		m_meshBundles.RemoveElement(bundleIndex);
	}

	void RecreateGraphicsPipelines()
	{
		for (auto& graphicsPipeline : m_graphicsPipelines)
			graphicsPipeline.Recreate(m_device, m_graphicsPipelineLayout, m_renderPass, m_shaderPath);
	}

protected:
	[[nodiscard]]
	std::optional<std::uint32_t> TryToGetPSOIndex(const ShaderName& fragmentShader) const noexcept
	{
		auto result = std::ranges::find_if(m_graphicsPipelines,
			[&fragmentShader](const Pipeline& pipeline)
			{
				return fragmentShader == pipeline.GetFragmentShader();
			});

		if (result != std::end(m_graphicsPipelines))
			return static_cast<std::uint32_t>(std::distance(std::begin(m_graphicsPipelines), result));
		else
			return {};
	}

	// Adds a new PSO, if one can't be found.
	std::uint32_t GetPSOIndex(const ShaderName& fragmentShader)
	{
		std::uint32_t psoIndex = 0u;

		auto oPSOIndex = TryToGetPSOIndex(fragmentShader);

		if (!oPSOIndex)
		{
			psoIndex = static_cast<std::uint32_t>(std::size(m_graphicsPipelines));

			Pipeline pipeline{};

			pipeline.Create(
				m_device, m_graphicsPipelineLayout, m_renderPass, m_shaderPath, fragmentShader
			);

			m_graphicsPipelines.emplace_back(std::move(pipeline));
		}
		else
			psoIndex = oPSOIndex.value();

		return psoIndex;
	}

	void BindPipeline(
		const ModelBundleType& modelBundle, const VKCommandBuffer& graphicsBuffer,
		size_t& previousPSOIndex
	) const noexcept {
		// PSO is more costly to bind, so the modelBundles are added in a way so they are sorted
		// by their PSO indices. And we only bind a new PSO, if the previous one was different.
		const size_t modelPSOIndex = modelBundle.GetPSOIndex();

		if (modelPSOIndex != previousPSOIndex)
		{
			m_graphicsPipelines[modelPSOIndex].Bind(graphicsBuffer);

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

protected:
	VkDevice                     m_device;
	MemoryManager*               m_memoryManager;
	VkPipelineLayout             m_graphicsPipelineLayout;
	VkRenderPass                 m_renderPass;
	std::wstring                 m_shaderPath;
	ModelBuffers                 m_modelBuffers;
	std::vector<Pipeline>        m_graphicsPipelines;
	ReusableVector<MeshManager>  m_meshBundles;
	std::vector<ModelBundleType> m_modelBundles;
	bool                         m_tempCopyNecessary;

	// The fragment and Vertex data are on different sets. So both can be 0u.
	static constexpr std::uint32_t s_modelBuffersFragmentBindingSlot = 0u;
	static constexpr std::uint32_t s_modelBuffersGraphicsBindingSlot = 0u;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ other.m_memoryManager },
		m_graphicsPipelineLayout{ other.m_graphicsPipelineLayout },
		m_renderPass{ other.m_renderPass },
		m_shaderPath{ std::move(other.m_shaderPath) },
		m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_graphicsPipelines{ std::move(other.m_graphicsPipelines) },
		m_meshBundles{ std::move(other.m_meshBundles) },
		m_modelBundles{ std::move(other.m_modelBundles) },
		m_tempCopyNecessary{ other.m_tempCopyNecessary }
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_device                 = other.m_device;
		m_memoryManager          = other.m_memoryManager;
		m_graphicsPipelineLayout = other.m_graphicsPipelineLayout;
		m_renderPass             = other.m_renderPass;
		m_shaderPath             = std::move(other.m_shaderPath);
		m_modelBuffers           = std::move(other.m_modelBuffers);
		m_graphicsPipelines      = std::move(other.m_graphicsPipelines);
		m_meshBundles            = std::move(other.m_meshBundles);
		m_modelBundles           = std::move(other.m_modelBundles);
		m_tempCopyNecessary      = other.m_tempCopyNecessary;

		return *this;
	}
};

class ModelManagerVSIndividual : public
	ModelManager
	<
		ModelManagerVSIndividual,
		GraphicsPipelineIndividualDraw,
		MeshManagerVertexShader,
		ModelBundleVSIndividual
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndividual,
			GraphicsPipelineIndividualDraw,
			MeshManagerVertexShader,
			ModelBundleVSIndividual
		>;
	friend class ModelManagerVSIndividualTest;

public:
	ModelManagerVSIndividual(
		VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3,
		std::uint32_t frameCount
	);

	void SetDescriptorBufferLayout(
		std::vector<VkDescriptorBuffer>& descriptorBuffers,
		size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
	);
	void SetDescriptorBuffer(
		std::vector<VkDescriptorBuffer>& descriptorBuffers,
		size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
	);

	void Draw(const VKCommandBuffer& graphicsBuffer) const noexcept;

	void CopyTempData(const VKCommandBuffer& transferCmdBuffer) noexcept;

private:
	static void _setGraphicsConstantRange(PipelineLayout& layout) noexcept;

	void ConfigureModelBundle(
		ModelBundleVSIndividual& modelBundleObj,
		std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle,
		TemporaryDataBufferGPU& tempBuffer
	) const noexcept;

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
	);

	void _updatePerFrame([[maybe_unused]]VkDeviceSize frameIndex) const noexcept {}
	// To create compute shader pipelines.
	void ShaderPathSet() {}

	[[nodiscard]]
	static ModelBuffers ConstructModelBuffers(
		VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount, QueueIndices3 queueIndices
	);

private:
	SharedBufferGPU m_vertexBuffer;
	SharedBufferGPU m_indexBuffer;

public:
	ModelManagerVSIndividual(const ModelManagerVSIndividual&) = delete;
	ModelManagerVSIndividual& operator=(const ModelManagerVSIndividual&) = delete;

	ModelManagerVSIndividual(ModelManagerVSIndividual&& other) noexcept
		: ModelManager{ std::move(other) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) }
	{}
	ModelManagerVSIndividual& operator=(ModelManagerVSIndividual&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_vertexBuffer        = std::move(other.m_vertexBuffer);
		m_indexBuffer         = std::move(other.m_indexBuffer);

		return *this;
	}
};

class ModelManagerVSIndirect : public
	ModelManager
	<
		ModelManagerVSIndirect,
		GraphicsPipelineIndirectDraw,
		MeshManagerVertexShader,
		ModelBundleVSIndirect
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndirect,
			GraphicsPipelineIndirectDraw,
			MeshManagerVertexShader,
			ModelBundleVSIndirect
		>;
	friend class ModelManagerVSIndirectTest;

	struct ConstantData
	{
		std::uint32_t modelCount;
	};

public:
	ModelManagerVSIndirect(
		VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
		QueueIndices3 queueIndices3, std::uint32_t frameCount
	);

	void ResetCounterBuffer(const VKCommandBuffer& computeCmdBuffer, size_t frameIndex) const noexcept;

	void SetComputePipelineLayout(VkPipelineLayout layout) noexcept;

	static void SetComputeConstantRange(PipelineLayout& layout) noexcept;

	void CopyTempBuffers(const VKCommandBuffer& transferBuffer) noexcept;

	void SetDescriptorBufferLayoutVS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
	) const noexcept;
	void SetDescriptorBufferVS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
	) const;

	void SetDescriptorBufferLayoutCS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
	) const noexcept;

	void SetDescriptorBufferCSOfModels(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
	) const;
	void SetDescriptorBufferCSOfMeshes(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
	) const;

	void Draw(size_t frameIndex, const VKCommandBuffer& graphicsBuffer) const noexcept;
	void Dispatch(const VKCommandBuffer& computeBuffer) const noexcept;

private:
	static void _setGraphicsConstantRange(PipelineLayout& layout) noexcept;

	void ConfigureModelBundle(
		ModelBundleVSIndirect& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle,
		TemporaryDataBufferGPU& tempBuffer
	);

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
	);

	void _updatePerFrame(VkDeviceSize frameIndex) const noexcept;

	// To create compute shader pipelines.
	void ShaderPathSet();

	void UpdateDispatchX() noexcept;
	void UpdateCounterResetValues();

	[[nodiscard]]
	static ModelBuffers ConstructModelBuffers(
		VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount, QueueIndices3 queueIndices
	);

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(ConstantData));
	}

private:
	StagingBufferManager*                 m_stagingBufferMan;
	std::vector<SharedBufferCPU>          m_argumentInputBuffers;
	std::vector<SharedBufferGPUWriteOnly> m_argumentOutputBuffers;
	std::vector<SharedBufferGPUWriteOnly> m_modelIndicesVSBuffers;
	SharedBufferCPU                       m_cullingDataBuffer;
	std::vector<SharedBufferGPUWriteOnly> m_counterBuffers;
	Buffer                                m_counterResetBuffer;
	MultiInstanceCPUBuffer<std::uint32_t> m_meshBundleIndexBuffer;
	SharedBufferGPU                       m_perModelDataCSBuffer;
	SharedBufferGPU                       m_vertexBuffer;
	SharedBufferGPU                       m_indexBuffer;
	SharedBufferGPU                       m_perMeshDataBuffer;
	SharedBufferGPU                       m_perMeshBundleDataBuffer;
	VkPipelineLayout                      m_pipelineLayoutCS;
	ComputePipeline                       m_computePipeline;
	QueueIndices3                         m_queueIndices3;
	std::uint32_t                         m_dispatchXCount;
	std::uint32_t                         m_argumentCount;

	// These CS models will have the data to be uploaded and the dispatching will be done on the Manager.
	std::vector<ModelBundleCSIndirect>    m_modelBundlesCS;

	// Vertex Shader ones
	static constexpr std::uint32_t s_modelIndicesVSBindingSlot      = 1u;

	// Compute Shader ones
	static constexpr std::uint32_t s_modelBuffersComputeBindingSlot = 0u;
	static constexpr std::uint32_t s_perModelDataCSBindingSlot      = 1u;
	static constexpr std::uint32_t s_argumentInputBufferBindingSlot = 2u;
	static constexpr std::uint32_t s_cullingDataBufferBindingSlot   = 3u;
	static constexpr std::uint32_t s_argumenOutputBindingSlot       = 4u;
	static constexpr std::uint32_t s_counterBindingSlot             = 5u;
	static constexpr std::uint32_t s_perMeshDataBindingSlot         = 6u;
	static constexpr std::uint32_t s_perMeshBundleDataBindingSlot   = 7u;
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
		m_stagingBufferMan{ other.m_stagingBufferMan },
		m_argumentInputBuffers{ std::move(other.m_argumentInputBuffers) },
		m_argumentOutputBuffers{ std::move(other.m_argumentOutputBuffers) },
		m_modelIndicesVSBuffers{ std::move(other.m_modelIndicesVSBuffers) },
		m_cullingDataBuffer{ std::move(other.m_cullingDataBuffer) },
		m_counterBuffers{ std::move(other.m_counterBuffers) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_meshBundleIndexBuffer{ std::move(other.m_meshBundleIndexBuffer) },
		m_perModelDataCSBuffer{ std::move(other.m_perModelDataCSBuffer) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) },
		m_perMeshDataBuffer{ std::move(other.m_perMeshDataBuffer) },
		m_perMeshBundleDataBuffer{ std::move(other.m_perMeshBundleDataBuffer) },
		m_pipelineLayoutCS{ other.m_pipelineLayoutCS },
		m_computePipeline{ std::move(other.m_computePipeline) },
		m_queueIndices3{ other.m_queueIndices3 },
		m_dispatchXCount{ other.m_dispatchXCount },
		m_argumentCount{ other.m_argumentCount },
		m_modelBundlesCS{ std::move(other.m_modelBundlesCS) }
	{}
	ModelManagerVSIndirect& operator=(ModelManagerVSIndirect&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_stagingBufferMan        = other.m_stagingBufferMan;
		m_argumentInputBuffers    = std::move(other.m_argumentInputBuffers);
		m_argumentOutputBuffers   = std::move(other.m_argumentOutputBuffers);
		m_modelIndicesVSBuffers   = std::move(other.m_modelIndicesVSBuffers);
		m_cullingDataBuffer       = std::move(other.m_cullingDataBuffer);
		m_counterBuffers          = std::move(other.m_counterBuffers);
		m_counterResetBuffer      = std::move(other.m_counterResetBuffer);
		m_meshBundleIndexBuffer   = std::move(other.m_meshBundleIndexBuffer);
		m_perModelDataCSBuffer    = std::move(other.m_perModelDataCSBuffer);
		m_vertexBuffer            = std::move(other.m_vertexBuffer);
		m_indexBuffer             = std::move(other.m_indexBuffer);
		m_perMeshDataBuffer       = std::move(other.m_perMeshDataBuffer);
		m_perMeshBundleDataBuffer = std::move(other.m_perMeshBundleDataBuffer);
		m_pipelineLayoutCS        = other.m_pipelineLayoutCS;
		m_computePipeline         = std::move(other.m_computePipeline);
		m_queueIndices3           = other.m_queueIndices3;
		m_dispatchXCount          = other.m_dispatchXCount;
		m_argumentCount           = other.m_argumentCount;
		m_modelBundlesCS          = std::move(other.m_modelBundlesCS);

		return *this;
	}
};

class ModelManagerMS : public
	ModelManager
	<
		ModelManagerMS,
		GraphicsPipelineMeshShader,
		MeshManagerMeshShader,
		ModelBundleMSIndividual
	>
{
	friend class ModelManager
		<
			ModelManagerMS,
			GraphicsPipelineMeshShader,
			MeshManagerMeshShader,
			ModelBundleMSIndividual
		>;
	friend class ModelManagerMSTest;

public:
	ModelManagerMS(
		VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
		QueueIndices3 queueIndices3, std::uint32_t frameCount
	);

	void SetDescriptorBufferLayout(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex, size_t fsSetLayoutIndex
	) const noexcept;

	// Should be called after a new Mesh has been added.
	void SetDescriptorBufferOfMeshes(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex
	) const;
	// Should be called after a new Model has been added.
	void SetDescriptorBufferOfModels(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex, size_t fsSetLayoutIndex
	) const;

	void CopyTempBuffers(const VKCommandBuffer& transferBuffer) noexcept;

	void Draw(const VKCommandBuffer& graphicsBuffer) const noexcept;

private:
	static void _setGraphicsConstantRange(PipelineLayout& layout) noexcept;

	void ConfigureModelBundle(
		ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
		std::shared_ptr<ModelBundle>&& modelBundle, TemporaryDataBufferGPU& tempBuffer
	);

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerMeshShader& meshManager, TemporaryDataBufferGPU& tempBuffer
	);

	void _updatePerFrame([[maybe_unused]]VkDeviceSize frameIndex) const noexcept {}
	// To create compute shader pipelines.
	void ShaderPathSet() {}

	[[nodiscard]]
	static ModelBuffers ConstructModelBuffers(
		VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount, QueueIndices3 queueIndices
	);

private:
	StagingBufferManager* m_stagingBufferMan;
	SharedBufferGPU       m_perMeshletDataBuffer;
	SharedBufferGPU       m_vertexBuffer;
	SharedBufferGPU       m_vertexIndicesBuffer;
	SharedBufferGPU       m_primIndicesBuffer;

	static constexpr std::uint32_t s_perMeshletBufferBindingSlot    = 1u;
	static constexpr std::uint32_t s_vertexBufferBindingSlot        = 2u;
	static constexpr std::uint32_t s_vertexIndicesBufferBindingSlot = 3u;
	static constexpr std::uint32_t s_primIndicesBufferBindingSlot   = 4u;

public:
	ModelManagerMS(const ModelManagerMS&) = delete;
	ModelManagerMS& operator=(const ModelManagerMS&) = delete;

	ModelManagerMS(ModelManagerMS&& other) noexcept
		: ModelManager{ std::move(other) },
		m_stagingBufferMan{ other.m_stagingBufferMan },
		m_perMeshletDataBuffer{ std::move(other.m_perMeshletDataBuffer) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_vertexIndicesBuffer{ std::move(other.m_vertexIndicesBuffer) },
		m_primIndicesBuffer{ std::move(other.m_primIndicesBuffer) }
	{}
	ModelManagerMS& operator=(ModelManagerMS&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_stagingBufferMan     = other.m_stagingBufferMan;
		m_perMeshletDataBuffer = std::move(other.m_perMeshletDataBuffer);
		m_vertexBuffer         = std::move(other.m_vertexBuffer);
		m_vertexIndicesBuffer  = std::move(other.m_vertexIndicesBuffer);
		m_primIndicesBuffer    = std::move(other.m_primIndicesBuffer);

		return *this;
	}
};
#endif
