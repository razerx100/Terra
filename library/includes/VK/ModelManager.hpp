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

#include <MeshManagerVertexShader.hpp>
#include <MeshManagerMeshShader.hpp>
#include <CommonBuffers.hpp>
#include <Model.hpp>
#include <Shader.hpp>

class ModelBundle
{
public:
	ModelBundle() : m_psoIndex{ 0u }, m_meshIndex{ 0u } {}

	void SetPSOIndex(std::uint32_t index) noexcept { m_psoIndex = index; }
	void SetMeshIndex(std::uint32_t index) noexcept { m_meshIndex = index; }

	[[nodiscard]]
	std::uint32_t GetPSOIndex() const noexcept { return m_psoIndex; }
	[[nodiscard]]
	std::uint32_t GetMeshIndex() const noexcept { return m_meshIndex; }

	[[nodiscard]]
	static VkDrawIndexedIndirectCommand GetDrawIndexedIndirectCommand(
		const std::shared_ptr<ModelVS>& model
	) noexcept;

protected:
	std::uint32_t m_psoIndex;
	std::uint32_t m_meshIndex;

public:
	ModelBundle(const ModelBundle&) = delete;
	ModelBundle& operator=(const ModelBundle&) = delete;

	ModelBundle(ModelBundle&& other) noexcept
		: m_psoIndex{ other.m_psoIndex }, m_meshIndex{ other.m_meshIndex }
	{}
	ModelBundle& operator=(ModelBundle&& other) noexcept
	{
		m_psoIndex  = other.m_psoIndex;
		m_meshIndex = other.m_meshIndex;

		return *this;
	}
};

class ModelBundleVSIndividual : public ModelBundle
{
public:
	struct ModelDetails
	{
		std::uint32_t                modelBufferIndex;
		VkDrawIndexedIndirectCommand indexedArguments;
	};

public:
	ModelBundleVSIndividual() : ModelBundle{}, m_modelDetails{} {}

	void AddModelDetails(const std::shared_ptr<ModelVS>& model, std::uint32_t modelBufferIndex) noexcept;
	void Draw(const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout) const noexcept;
	void Draw(
		const VKCommandBuffer& graphicsBuffer, const PipelineLayout& pipelineLayout
	) const noexcept {
		Draw(graphicsBuffer, pipelineLayout.Get());
	}

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(std::uint32_t));
	}

	[[nodiscard]]
	const std::vector<ModelDetails>& GetDetails() const noexcept { return m_modelDetails; }

	[[nodiscard]]
	std::uint64_t GetID() const noexcept
	{
		// If there is no index, return an invalid one. We might have uint32_t::max models,
		// so return uint64_t::max as the invalid index. Then, the model indices should be unique
		// so, just returning the first one should be enough to identify the bundle, as no other bundles
		// should have it.
		if (!std::empty(m_modelDetails))
			return m_modelDetails.front().modelBufferIndex;
		else
			return std::numeric_limits<std::uint64_t>::max();
	}

private:
	std::vector<ModelDetails> m_modelDetails;

public:
	ModelBundleVSIndividual(const ModelBundleVSIndividual&) = delete;
	ModelBundleVSIndividual& operator=(const ModelBundleVSIndividual&) = delete;

	ModelBundleVSIndividual(ModelBundleVSIndividual&& other) noexcept
		: ModelBundle{ std::move(other) }, m_modelDetails{ std::move(other.m_modelDetails) }
	{}
	ModelBundleVSIndividual& operator=(ModelBundleVSIndividual&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelDetails       = std::move(other.m_modelDetails);

		return *this;
	}
};

class ModelBundleMS : public ModelBundle
{
public:
	struct ModelDetails
	{
		std::uint32_t modelBufferIndex;
		// Since there could be meshlets of multiple Models, we need the starting point of each.
		std::uint32_t meshletOffset;
		std::uint32_t threadGroupCountX;
	};

	struct RenderData
	{
		std::uint32_t modelBufferIndex;
		std::uint32_t meshletOffset;
	};

	struct TempData
	{
		std::vector<Meshlet> meshlets;
	};

public:
	ModelBundleMS()
		: ModelBundle{}, m_modelDetails{}, m_meshletSharedData{ nullptr, 0u, 0u }, m_meshlets{}
	{}

	// Need this one as non-const, since I am gonna move the meshlets.
	void AddModelDetails(std::shared_ptr<ModelMS>& model, std::uint32_t modelBufferIndex) noexcept;
	void CreateBuffers(
		StagingBufferManager& stagingBufferMan, SharedBuffer& meshletSharedBuffer,
		std::deque<TempData>& tempDataContainer
	);

	void Draw(
		const VKCommandBuffer& graphicsBuffer, const PipelineLayout& pipelineLayout
	) const noexcept;

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(RenderData));
	}

	[[nodiscard]]
	const std::vector<ModelDetails>& GetDetails() const noexcept { return m_modelDetails; }

	[[nodiscard]]
	std::uint64_t GetID() const noexcept
	{
		// If there is no index, return an invalid one. We might have uint32_t::max models,
		// so return uint64_t::max as the invalid index. Then, the model indices should be unique
		// so, just returning the first one should be enough to identify the bundle, as no other bundles
		// should have it.
		if (!std::empty(m_modelDetails))
			return m_modelDetails.front().modelBufferIndex;
		else
			return std::numeric_limits<std::uint64_t>::max();
	}

	[[nodiscard]]
	const SharedBufferData& GetMeshletSharedData() const noexcept { return m_meshletSharedData; }

private:
	std::vector<ModelDetails> m_modelDetails;
	SharedBufferData          m_meshletSharedData;
	std::vector<Meshlet>      m_meshlets;

	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkExtMeshShader
	};

public:
	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{ return s_requiredExtensions; }

	ModelBundleMS(const ModelBundleMS&) = delete;
	ModelBundleMS& operator=(const ModelBundleMS&) = delete;

	ModelBundleMS(ModelBundleMS&& other) noexcept
		: ModelBundle{ std::move(other) },
		m_modelDetails{ std::move(other.m_modelDetails) },
		m_meshletSharedData{ other.m_meshletSharedData },
		m_meshlets{ std::move(other.m_meshlets) }
	{}
	ModelBundleMS& operator=(ModelBundleMS&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelDetails       = std::move(other.m_modelDetails);
		m_meshletSharedData  = other.m_meshletSharedData;
		m_meshlets           = std::move(other.m_meshlets);

		return *this;
	}
};

class ModelBundleCSIndirect
{
public:
	struct CullingData
	{
		std::uint32_t     commandCount;
		std::uint32_t     commandOffset;// Next Vec2 starts at 8bytes offset
		DirectX::XMFLOAT2 xBounds;
		DirectX::XMFLOAT2 yBounds;
		DirectX::XMFLOAT2 zBounds;
	};

	struct TempData
	{
		std::vector<VkDrawIndexedIndirectCommand> indirectArguments;
		std::vector<std::uint32_t>                modelBundleIndices;
		std::unique_ptr<CullingData>              cullingData;
	};

public:
	ModelBundleCSIndirect();

	void AddModelDetails(const std::shared_ptr<ModelVS>& model) noexcept;
	void CreateBuffers(
		StagingBufferManager& stagingBufferMan, SharedBuffer& argumentInputSharedBuffer,
		SharedBuffer& cullingSharedBuffer, SharedBuffer& modelBundleIndexSharedBuffer,
		std::deque<TempData>& tempDataContainer
	);

	void SetID(std::uint32_t bundleID) noexcept { m_bundleID = bundleID; }

	[[nodiscard]]
	std::uint32_t GetID() const noexcept { return m_bundleID; }

	[[nodiscard]]
	// Must be called after the buffers have been created.
	std::uint32_t GetModelBundleIndex() const noexcept
	{
		return static_cast<std::uint32_t>(m_cullingSharedData.offset / sizeof(CullingData));
	}

	[[nodiscard]]
	const SharedBufferData& GetArgumentInputSharedData() const noexcept
	{
		return m_argumentInputSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetCullingSharedData() const noexcept { return m_cullingSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetModelBundleIndexSharedData() const noexcept
	{
		return m_modelBundleIndexSharedData;
	}

private:
	SharedBufferData                          m_argumentInputSharedData;
	SharedBufferData                          m_cullingSharedData;
	SharedBufferData                          m_modelBundleIndexSharedData;
	std::vector<VkDrawIndexedIndirectCommand> m_indirectArguments;
	std::unique_ptr<CullingData>              m_cullingData;
	std::uint32_t                             m_bundleID;

	static constexpr DirectX::XMFLOAT2 XBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 YBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 ZBOUNDS = { 1.f, -1.f };

public:
	ModelBundleCSIndirect(const ModelBundleCSIndirect&) = delete;
	ModelBundleCSIndirect& operator=(const ModelBundleCSIndirect&) = delete;

	ModelBundleCSIndirect(ModelBundleCSIndirect&& other) noexcept
		: m_argumentInputSharedData{ other.m_argumentInputSharedData },
		m_cullingSharedData{ other.m_cullingSharedData },
		m_modelBundleIndexSharedData{ other.m_modelBundleIndexSharedData },
		m_indirectArguments{ std::move(other.m_indirectArguments) },
		m_cullingData{ std::move(other.m_cullingData) },
		m_bundleID{ other.m_bundleID }
	{}
	ModelBundleCSIndirect& operator=(ModelBundleCSIndirect&& other) noexcept
	{
		m_argumentInputSharedData    = other.m_argumentInputSharedData;
		m_cullingSharedData          = other.m_cullingSharedData;
		m_modelBundleIndexSharedData = other.m_modelBundleIndexSharedData;
		m_indirectArguments          = std::move(other.m_indirectArguments);
		m_cullingData                = std::move(other.m_cullingData);
		m_bundleID                   = other.m_bundleID;

		return *this;
	}
};

class ModelBundleVSIndirect : public ModelBundle
{
public:
	ModelBundleVSIndirect();

	void AddModelDetails(std::uint32_t modelBufferIndex) noexcept;

	void CreateBuffers(
		StagingBufferManager& stagingBufferMan,
		std::vector<SharedBuffer>& argumentOutputSharedBuffers,
		std::vector<SharedBuffer>& counterSharedBuffers, SharedBuffer& modelIndicesBuffer
	);
	void Draw(const VKCommandBuffer& graphicsBuffer) const noexcept;

	[[nodiscard]]
	const std::vector<std::uint32_t>& GetModelIndices() const noexcept { return m_modelIndices; }

	[[nodiscard]]
	std::uint64_t GetID() const noexcept
	{
		// If there is no index, return an invalid one. We might have uint32_t::max models,
		// so return uint64_t::max as the invalid index. Then, the model indices should be unique
		// so, just returning the first one should be enough to identify the bundle, as no other bundles
		// should have it.
		if (!std::empty(m_modelIndices))
			return m_modelIndices.front();
		else
			return std::numeric_limits<std::uint64_t>::max();
	}

	[[nodiscard]]
	std::uint32_t GetModelCount() const noexcept { return m_modelCount; }
	[[nodiscard]]
	const SharedBufferData& GetArgumentOutputSharedData() const noexcept
	{
		return m_argumentOutputSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetCounterSharedData() const noexcept { return m_counterSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetModelIndicesSharedData() const noexcept
	{
		return m_modelIndicesSharedData;
	}

	[[nodiscard]]
	static VkDeviceSize GetCounterBufferSize() noexcept { return s_counterBufferSize; }

private:
	std::uint32_t              m_modelCount;
	SharedBufferData           m_argumentOutputSharedData;
	SharedBufferData           m_counterSharedData;
	SharedBufferData           m_modelIndicesSharedData;

	// I am gonna use the DrawIndex in the Vertex shader and the thread Index in the Compute shader
	// to index into this buffer and that will give us the actual model index.
	std::vector<std::uint32_t> m_modelIndices;

	inline static VkDeviceSize s_counterBufferSize = static_cast<VkDeviceSize>(sizeof(std::uint32_t));

public:
	ModelBundleVSIndirect(const ModelBundleVSIndirect&) = delete;
	ModelBundleVSIndirect& operator=(const ModelBundleVSIndirect&) = delete;

	ModelBundleVSIndirect(ModelBundleVSIndirect&& other) noexcept
		: ModelBundle{ std::move(other) },
		m_modelCount{ other.m_modelCount },
		m_argumentOutputSharedData{ other.m_argumentOutputSharedData },
		m_counterSharedData{ other.m_counterSharedData },
		m_modelIndicesSharedData{ other.m_modelIndicesSharedData },
		m_modelIndices{ std::move(other.m_modelIndices) }
	{}
	ModelBundleVSIndirect& operator=(ModelBundleVSIndirect&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelCount               = other.m_modelCount;
		m_argumentOutputSharedData = other.m_argumentOutputSharedData;
		m_counterSharedData        = other.m_counterSharedData;
		m_modelIndicesSharedData   = other.m_modelIndicesSharedData;
		m_modelIndices             = std::move(other.m_modelIndices);

		return *this;
	}
};

class ModelBuffers : public ReusableVkBuffer<ModelBuffers, std::shared_ptr<Model>>
{
	friend class ReusableVkBuffer<ModelBuffers, std::shared_ptr<Model>>;

public:
	ModelBuffers(VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount)
		: ReusableVkBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_fragmentModelBuffers{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_modelBuffersInstanceSize{ 0u }, m_modelBuffersFragmentInstanceSize{ 0u },
		m_bufferInstanceCount{ frameCount }
	{}

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot
	) const;
	void SetFragmentDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot
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
	Buffer        m_fragmentModelBuffers;
	VkDeviceSize  m_modelBuffersInstanceSize;
	VkDeviceSize  m_modelBuffersFragmentInstanceSize;
	std::uint32_t m_bufferInstanceCount;

public:
	ModelBuffers(const ModelBuffers&) = delete;
	ModelBuffers& operator=(const ModelBuffers&) = delete;

	ModelBuffers(ModelBuffers&& other) noexcept
		: ReusableVkBuffer{ std::move(other) },
		m_fragmentModelBuffers{ std::move(other.m_fragmentModelBuffers) },
		m_modelBuffersInstanceSize{ other.m_modelBuffersInstanceSize },
		m_modelBuffersFragmentInstanceSize{ other.m_modelBuffersFragmentInstanceSize },
		m_bufferInstanceCount{ other.m_bufferInstanceCount }
	{}
	ModelBuffers& operator=(ModelBuffers&& other) noexcept
	{
		ReusableVkBuffer::operator=(std::move(other));
		m_fragmentModelBuffers             = std::move(other.m_fragmentModelBuffers);
		m_modelBuffersInstanceSize         = other.m_modelBuffersInstanceSize;
		m_modelBuffersFragmentInstanceSize = other.m_modelBuffersFragmentInstanceSize;
		m_bufferInstanceCount              = other.m_bufferInstanceCount;

		return *this;
	}
};

template<
	class Derived,
	class Pipeline,
	class MeshManager,
	class MeshType,
	class ModelBundleType,
	class ModelType,
	bool  TempData, bool meshBounds,
	bool OverloadSetMesh
>
class ModelManager
{
	using MeshTempData = std::conditional_t<
		meshBounds ,typename MeshManager::TempDataBounds, typename MeshManager::TempData
	>;

public:
	ModelManager(VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_graphicsPipelineLayout{ device }, m_renderPass{ nullptr }, m_shaderPath{},
		m_modelBuffers{ device, memoryManager, frameCount }, m_graphicsPipelines{},
		m_meshBundles{}, m_meshBoundsBuffer{
			device, memoryManager,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
		}, m_meshBundleTempData{}, m_modelBundles{}, m_tempDataMutex{}
	{}

	// The layout should be the same across the multiple descriptors for each frame.
	void CreatePipelineLayout(const VkDescriptorBuffer& descriptorBuffer)
	{
		static_cast<Derived*>(this)->CreatePipelineLayoutImpl(descriptorBuffer);
	}

	[[nodiscard]]
	const PipelineLayout& GetGraphicsPipelineLayout() const noexcept
	{
		return m_graphicsPipelineLayout;
	}

	void UpdatePerFrame(VkDeviceSize frameIndex) const noexcept
	{
		m_modelBuffers.Update(frameIndex);
	}

	void SetRenderPass(VKRenderPass* renderPass) noexcept
	{
		m_renderPass = renderPass;
	}

	void SetShaderPath(std::wstring shaderPath) noexcept
	{
		m_shaderPath = std::move(shaderPath);
	}

	[[nodiscard]]
	std::uint32_t AddModel(std::shared_ptr<ModelType>&& model, const ShaderName& fragmentShader)
	{
		// This is necessary since the model buffers needs an Rvalue ref and returns the modelIndex,
		// which is necessary to add MeshDetails. Which can't be done without the modelIndex.
		std::shared_ptr<ModelType> tempModel = model;

		const size_t modelIndex = m_modelBuffers.Add(std::move(model));

		auto dvThis = static_cast<Derived*>(this);

		ModelBundleType modelBundle{};
		dvThis->ConfigureModel(modelBundle, modelIndex, tempModel);

		const std::uint32_t psoIndex = GetPSOIndex(fragmentShader);

		modelBundle.SetPSOIndex(psoIndex);

		const auto bundleID = static_cast<std::uint32_t>(modelBundle.GetID());

		AddModelBundle(std::move(modelBundle));

		return bundleID;
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelType>>&& modelBundle, const ShaderName& fragmentShader
	) {
		const size_t modelCount = std::size(modelBundle);

		if (modelCount)
		{
			std::vector<std::shared_ptr<Model>> tempModelBundle{ modelCount, nullptr };

			for (size_t index = 0u; index < modelCount; ++index)
				tempModelBundle.at(index) = std::static_pointer_cast<Model>(modelBundle.at(index));

			const std::vector<size_t> modelIndices
				= m_modelBuffers.AddMultiple(std::move(tempModelBundle));

			auto dvThis = static_cast<Derived*>(this);

			ModelBundleType modelBundleObj{};

			dvThis->ConfigureModelBundle(modelBundleObj, modelIndices, modelBundle);

			const std::uint32_t psoIndex = GetPSOIndex(fragmentShader);

			modelBundleObj.SetPSOIndex(psoIndex);

			const auto bundleID = static_cast<std::uint32_t>(modelBundleObj.GetID());

			AddModelBundle(std::move(modelBundleObj));

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

	void SetMeshIndex(std::uint32_t modelBundleID, std::uint32_t meshBundleID)
	{
		auto modelBundle = GetModelBundle(modelBundleID);

		if (modelBundle != std::end(m_modelBundles))
		{
			modelBundle->SetMeshIndex(meshBundleID);

			if constexpr (OverloadSetMesh)
				static_cast<Derived*>(this)->_setMeshIndex(
					std::distance(std::begin(m_modelBundles), modelBundle), meshBundleID
				);
		}
	}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(
		std::unique_ptr<MeshType> meshBundle, StagingBufferManager& stagingBufferMan
	) {
		MeshManager meshManager{};

		{
			// Since the mesh bundle temp data will be cleaned from a different thread.
			// And there is also a good chance that the ConfugureMeshBundle will also
			// have the possiblity to create tempData from some SharedBuffer.
			std::scoped_lock<std::mutex> tempDataLock{ m_tempDataMutex };

			static_cast<Derived*>(this)->ConfigureMeshBundle(
				std::move(meshBundle), stagingBufferMan, meshManager
			);
		}

		auto meshIndex = m_meshBundles.Add(std::move(meshManager));

		return static_cast<std::uint32_t>(meshIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
	{
		static_cast<Derived*>(this)->ConfigureRemoveMesh(bundleIndex);

		// It is okay to use the non-clear function based RemoveElement, as I will be
		// moving the Buffers out as SharedBuffer.
		m_meshBundles.RemoveElement(bundleIndex);
	}

	void CleanUpTempData() noexcept
	{
		std::scoped_lock<std::mutex> tempDataLock{ m_tempDataMutex };

		m_meshBundleTempData = std::deque<MeshTempData>{};

		if constexpr (TempData)
			static_cast<Derived*>(this)->_cleanUpTempData();
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
				m_device, m_graphicsPipelineLayout, *m_renderPass, m_shaderPath, fragmentShader
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
			m_graphicsPipelines.at(modelPSOIndex).Bind(graphicsBuffer);

			previousPSOIndex = modelPSOIndex;
		}
	}

	void BindMesh(
		const ModelBundleType& modelBundle, const VKCommandBuffer& graphicsBuffer
	) const noexcept {
		// We could also do the same for the meshes, but we can only sort by a single thing and
		// PSO binding is more costly, so it is better to sort the models by their PSO indices.
		const size_t meshIndex = modelBundle.GetMeshIndex();

		m_meshBundles.at(meshIndex).Bind(graphicsBuffer);
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

	void MoveTempData(ModelManager& other) noexcept
	{
		std::scoped_lock<std::mutex> tempDataLock{ other.m_tempDataMutex };
		m_meshBundleTempData = std::move(other.m_meshBundleTempData);
	}

protected:
	VkDevice                     m_device;
	MemoryManager*               m_memoryManager;
	PipelineLayout               m_graphicsPipelineLayout;
	VKRenderPass*                m_renderPass;
	std::wstring                 m_shaderPath;
	ModelBuffers                 m_modelBuffers;
	std::vector<Pipeline>        m_graphicsPipelines;
	ReusableVector<MeshManager>  m_meshBundles;
	// Configure this buffer in the child class, if desired.
	SharedBuffer                 m_meshBoundsBuffer;
	std::deque<MeshTempData>     m_meshBundleTempData;
	std::vector<ModelBundleType> m_modelBundles;
	std::mutex                   m_tempDataMutex;

	// Need to update this when I update the shaders.
	static constexpr std::uint32_t s_modelBuffersGraphicsBindingSlot = 0u;
	static constexpr std::uint32_t s_modelBuffersFragmentBindingSlot = 1u;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ other.m_memoryManager },
		m_graphicsPipelineLayout{ std::move(other.m_graphicsPipelineLayout) },
		m_renderPass{ other.m_renderPass },
		m_shaderPath{ std::move(other.m_shaderPath) },
		m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_graphicsPipelines{ std::move(other.m_graphicsPipelines) },
		m_meshBundles{ std::move(other.m_meshBundles) },
		m_meshBoundsBuffer{ std::move(other.m_meshBoundsBuffer) },
		// Can't move the data here, as this resource will be used from multiple threads.
		m_meshBundleTempData{},
		m_modelBundles{ std::move(other.m_modelBundles) },
		// We can't move mutices, so create a new one.
		m_tempDataMutex{}
	{
		MoveTempData(other);
	}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_device                 = other.m_device;
		m_memoryManager          = other.m_memoryManager;
		m_graphicsPipelineLayout = std::move(other.m_graphicsPipelineLayout);
		m_renderPass             = other.m_renderPass;
		m_shaderPath             = std::move(other.m_shaderPath);
		m_modelBuffers           = std::move(other.m_modelBuffers);
		m_graphicsPipelines      = std::move(other.m_graphicsPipelines);
		m_meshBundles            = std::move(other.m_meshBundles);
		m_meshBoundsBuffer       = std::move(other.m_meshBoundsBuffer);
		m_modelBundles           = std::move(other.m_modelBundles);

		MoveTempData(other);

		return *this;
	}
};

class ModelManagerVSIndividual : public
	ModelManager
	<
		ModelManagerVSIndividual,
		GraphicsPipelineIndividualDraw,
		MeshManagerVertexShader, MeshBundleVS,
		ModelBundleVSIndividual, ModelVS,
		true, false, false
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndividual,
			GraphicsPipelineIndividualDraw,
			MeshManagerVertexShader, MeshBundleVS,
			ModelBundleVSIndividual, ModelVS,
			true, false, false
		>;
	friend class ModelManagerVSIndividualTest;
public:
	ModelManagerVSIndividual(VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount);

	void SetDescriptorBufferLayout(std::vector<VkDescriptorBuffer>& descriptorBuffers);
	void SetDescriptorBuffer(std::vector<VkDescriptorBuffer>& descriptorBuffers);

	void Draw(const VKCommandBuffer& graphicsBuffer) const noexcept;

	void CopyTempData(const VKCommandBuffer& transferCmdBuffer) const noexcept;

private:
	void CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer);

	void ConfigureModel(
		ModelBundleVSIndividual& modelBundleObj,
		size_t modelIndex, const std::shared_ptr<ModelVS>& model
	) const noexcept;
	void ConfigureModelBundle(
		ModelBundleVSIndividual& modelBundleObj,
		const std::vector<size_t>& modelIndices,
		const std::vector<std::shared_ptr<ModelVS>>& modelBundle
	) const noexcept;

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerVertexShader& meshManager
	);

private:
	void _cleanUpTempData() noexcept;

private:
	SharedBuffer m_vertexBuffer;
	SharedBuffer m_indexBuffer;

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
		MeshManagerVertexShader, MeshBundleVS,
		ModelBundleVSIndirect, ModelVS,
		true, true, true
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndirect,
			GraphicsPipelineIndirectDraw,
			MeshManagerVertexShader, MeshBundleVS,
			ModelBundleVSIndirect, ModelVS,
			true, true, true
		>;
	friend class ModelManagerVSIndirectTest;

	using CSIndirectTempData = ModelBundleCSIndirect::TempData;
public:
	ModelManagerVSIndirect(
		VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
		QueueIndices3 queueIndices3, std::uint32_t frameCount
	);

	void ResetCounterBuffer(
		const VKCommandBuffer& transferBuffer, VkDeviceSize frameIndex
	) const noexcept;

	void CreatePipelineCS(const VkDescriptorBuffer& descriptorBuffer);

	[[nodiscard]]
	const PipelineLayout& GetComputePipelineLayout() const noexcept
	{
		return m_pipelineLayoutCS;
	}

	void CopyTempBuffers(const VKCommandBuffer& transferBuffer) noexcept;

	void SetDescriptorBufferLayoutVS(std::vector<VkDescriptorBuffer>& descriptorBuffers) const noexcept;
	void SetDescriptorBufferVS(std::vector<VkDescriptorBuffer>& descriptorBuffers) const;

	void SetDescriptorBufferLayoutCS(std::vector<VkDescriptorBuffer>& descriptorBuffers) const noexcept;

	void SetDescriptorBufferCSOfModels(std::vector<VkDescriptorBuffer>& descriptorBuffers) const;
	void SetDescriptorBufferCSOfMeshes(std::vector<VkDescriptorBuffer>& descriptorBuffers) const;

	void Draw(const VKCommandBuffer& graphicsBuffer) const noexcept;
	void Dispatch(const VKCommandBuffer& computeBuffer) const noexcept;

private:
	void CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer);

	void ConfigureModel(
		ModelBundleVSIndirect& modelBundleObj, size_t modelIndex, const std::shared_ptr<ModelVS>& model
	);
	void ConfigureModelBundle(
		ModelBundleVSIndirect& modelBundleObj, const std::vector<size_t>& modelIndices,
		const std::vector<std::shared_ptr<ModelVS>>& modelBundle
	);

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerVertexShader& meshManager
	);

	void _cleanUpTempData() noexcept;
	void _setMeshIndex(size_t modelBundelVSIndex, std::uint32_t meshBundleID);

	void UpdateDispatchX() noexcept;
	void UpdateCounterResetValues();

	void MoveTempCSData(ModelManagerVSIndirect& other) noexcept
	{
		std::scoped_lock<std::mutex> tempDataLock{ other.m_tempDataMutex };
		m_modelBundleCSTempData = std::move(other.m_modelBundleCSTempData);
	}

private:
	StagingBufferManager*              m_stagingBufferMan;
	SharedBuffer                       m_argumentInputBuffer;
	std::vector<SharedBuffer>          m_argumentOutputBuffers;
	SharedBuffer                       m_cullingDataBuffer;
	std::vector<SharedBuffer>          m_counterBuffers;
	Buffer                             m_counterResetBuffer;
	ReusableCPUBuffer<std::uint32_t>   m_meshIndexBuffer;
	SharedBuffer                       m_modelIndicesBuffer;
	SharedBuffer                       m_vertexBuffer;
	SharedBuffer                       m_indexBuffer;
	SharedBuffer                       m_modelBundleIndexBuffer;
	PipelineLayout                     m_pipelineLayoutCS;
	ComputePipeline                    m_computePipeline;
	QueueIndices3                      m_queueIndices3;
	std::uint32_t                      m_dispatchXCount;
	std::uint32_t                      m_argumentCount;
	bool                               m_tempGPUDataCopied;

	// These CS models will have the data to be uploaded and the dispatching will be done on the Manager.
	std::vector<ModelBundleCSIndirect> m_modelBundlesCS;
	std::deque<CSIndirectTempData>     m_modelBundleCSTempData;

	// Need to update these when I update the shaders.
	// Vertex Shader ones
	static constexpr std::uint32_t s_modelIndicesVSBindingSlot      = 2u;

	// Compute Shader ones
	static constexpr std::uint32_t s_modelBuffersComputeBindingSlot = 0u;
	static constexpr std::uint32_t s_modelIndicesCSBindingSlot      = 1u;
	static constexpr std::uint32_t s_argumentInputBufferBindingSlot = 2u;
	static constexpr std::uint32_t s_cullingDataBufferBindingSlot   = 3u;
	static constexpr std::uint32_t s_argumenOutputBindingSlot       = 4u;
	static constexpr std::uint32_t s_counterBindingSlot             = 5u;
	static constexpr std::uint32_t s_modelBundleIndexBindingSlot    = 6u;
	static constexpr std::uint32_t s_meshBoundingBindingSlot        = 7u;
	static constexpr std::uint32_t s_meshIndicesBindingSlot         = 8u;

	// Each Compute Thread Group should have 64 threads.
	static constexpr float THREADBLOCKSIZE = 64.f;

public:
	ModelManagerVSIndirect(const ModelManagerVSIndirect&) = delete;
	ModelManagerVSIndirect& operator=(const ModelManagerVSIndirect&) = delete;

	ModelManagerVSIndirect(ModelManagerVSIndirect&& other) noexcept
		: ModelManager{ std::move(other) },
		m_stagingBufferMan{ other.m_stagingBufferMan },
		m_argumentInputBuffer{ std::move(other.m_argumentInputBuffer) },
		m_argumentOutputBuffers{ std::move(other.m_argumentOutputBuffers) },
		m_cullingDataBuffer{ std::move(other.m_cullingDataBuffer) },
		m_counterBuffers{ std::move(other.m_counterBuffers) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_meshIndexBuffer{ std::move(other.m_meshIndexBuffer) },
		m_modelIndicesBuffer{ std::move(other.m_modelIndicesBuffer) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) },
		m_modelBundleIndexBuffer{ std::move(other.m_modelBundleIndexBuffer) },
		m_pipelineLayoutCS{ std::move(other.m_pipelineLayoutCS) },
		m_computePipeline{ std::move(other.m_computePipeline) },
		m_queueIndices3{ other.m_queueIndices3 },
		m_dispatchXCount{ other.m_dispatchXCount },
		m_argumentCount{ other.m_argumentCount },
		m_tempGPUDataCopied{ other.m_tempGPUDataCopied },
		m_modelBundlesCS{ std::move(other.m_modelBundlesCS) },
		// Can't move the data here, as this resource will be used from multiple threads.
		m_modelBundleCSTempData{}
	{
		MoveTempCSData(other);
	}
	ModelManagerVSIndirect& operator=(ModelManagerVSIndirect&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_stagingBufferMan       = other.m_stagingBufferMan;
		m_argumentInputBuffer    = std::move(other.m_argumentInputBuffer);
		m_argumentOutputBuffers  = std::move(other.m_argumentOutputBuffers);
		m_cullingDataBuffer      = std::move(other.m_cullingDataBuffer);
		m_counterBuffers         = std::move(other.m_counterBuffers);
		m_counterResetBuffer     = std::move(other.m_counterResetBuffer);
		m_meshIndexBuffer        = std::move(other.m_meshIndexBuffer);
		m_modelIndicesBuffer     = std::move(other.m_modelIndicesBuffer);
		m_vertexBuffer           = std::move(other.m_vertexBuffer);
		m_indexBuffer            = std::move(other.m_indexBuffer);
		m_modelBundleIndexBuffer = std::move(other.m_modelBundleIndexBuffer);
		m_pipelineLayoutCS       = std::move(other.m_pipelineLayoutCS);
		m_computePipeline        = std::move(other.m_computePipeline);
		m_queueIndices3          = other.m_queueIndices3;
		m_dispatchXCount         = other.m_dispatchXCount;
		m_argumentCount          = other.m_argumentCount;
		m_tempGPUDataCopied      = other.m_tempGPUDataCopied;
		m_modelBundlesCS         = std::move(other.m_modelBundlesCS);

		MoveTempCSData(other);

		return *this;
	}
};

class ModelManagerMS : public
	ModelManager
	<
		ModelManagerMS,
		GraphicsPipelineMeshShader,
		MeshManagerMeshShader, MeshBundleMS,
		ModelBundleMS, ModelMS,
		true, false, false
	>
{
	friend class ModelManager
		<
			ModelManagerMS,
			GraphicsPipelineMeshShader,
			MeshManagerMeshShader, MeshBundleMS,
			ModelBundleMS, ModelMS,
			true, false, false
		>;
	friend class ModelManagerMSTest;

	using MSBundleTempData = ModelBundleMS::TempData;

public:
	ModelManagerMS(
		VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
		std::uint32_t frameCount
	);

	void SetDescriptorBufferLayout(std::vector<VkDescriptorBuffer>& descriptorBuffers) const noexcept;

	// Should be called after a new Mesh has been added.
	void SetDescriptorBufferOfMeshes(std::vector<VkDescriptorBuffer>& descriptorBuffers) const;
	// Should be called after a new Model has been added.
	void SetDescriptorBufferOfModels(std::vector<VkDescriptorBuffer>& descriptorBuffers) const;

	void Draw(const VKCommandBuffer& graphicsBuffer) const noexcept;

private:
	void CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer);
	void ConfigureModel(
		ModelBundleMS& modelBundleObj, size_t modelIndex, std::shared_ptr<ModelMS>& model
	);
	void ConfigureModelBundle(
		ModelBundleMS& modelBundleObj, const std::vector<size_t>& modelIndices,
		std::vector<std::shared_ptr<ModelMS>>& modelBundle
	);

	void ConfigureModelRemove(size_t bundleIndex) noexcept;
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;
	void ConfigureMeshBundle(
		std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
		MeshManagerMeshShader& meshManager
	);

	void _cleanUpTempData() noexcept;

	void MoveTempMSData(ModelManagerMS& other) noexcept
	{
		std::scoped_lock<std::mutex> tempDataLock{ other.m_tempDataMutex };
		m_modelBundleTempData = std::move(other.m_modelBundleTempData);
	}

private:
	StagingBufferManager*        m_stagingBufferMan;
	SharedBuffer                 m_meshletBuffer;
	SharedBuffer                 m_vertexBuffer;
	SharedBuffer                 m_vertexIndicesBuffer;
	SharedBuffer                 m_primIndicesBuffer;
	std::deque<MSBundleTempData> m_modelBundleTempData;

	static constexpr std::uint32_t s_meshletBufferBindingSlot       = 2u;
	static constexpr std::uint32_t s_vertexBufferBindingSlot        = 3u;
	static constexpr std::uint32_t s_vertexIndicesBufferBindingSlot = 4u;
	static constexpr std::uint32_t s_primIndicesBufferBindingSlot   = 5u;

public:
	ModelManagerMS(const ModelManagerMS&) = delete;
	ModelManagerMS& operator=(const ModelManagerMS&) = delete;

	ModelManagerMS(ModelManagerMS&& other) noexcept
		: ModelManager{ std::move(other) },
		m_stagingBufferMan{ other.m_stagingBufferMan },
		m_meshletBuffer{ std::move(other.m_meshletBuffer) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_vertexIndicesBuffer{ std::move(other.m_vertexIndicesBuffer) },
		m_primIndicesBuffer{ std::move(other.m_primIndicesBuffer) },
		// Can't move the data here, as this resource will be used from multiple threads.
		m_modelBundleTempData{}
	{
		MoveTempMSData(other);
	}
	ModelManagerMS& operator=(ModelManagerMS&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_stagingBufferMan    = other.m_stagingBufferMan;
		m_meshletBuffer       = std::move(other.m_meshletBuffer);
		m_vertexBuffer        = std::move(other.m_vertexBuffer);
		m_vertexIndicesBuffer = std::move(other.m_vertexIndicesBuffer);
		m_primIndicesBuffer   = std::move(other.m_primIndicesBuffer);

		MoveTempMSData(other);

		return *this;
	}
};
#endif
