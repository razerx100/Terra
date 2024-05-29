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
#include <ReusableVkBuffer.hpp>
#include <GraphicsPipelineVertexShader.hpp>
#include <ComputePipeline.hpp>

#include <MeshManagerVertexShader.hpp>
#include <CommonBuffers.hpp>
#include <Model.hpp>

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

public:
	ModelBundleMS(VkDevice device, MemoryManager* memoryManager)
		: ModelBundle{}, m_modelDetails{},
		m_meshletBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
		m_meshlets{}
	{}

	// Need this one as non-const, since I am gonna move the meshlets.
	void AddModelDetails(std::shared_ptr<ModelMS>& model, std::uint32_t modelBufferIndex) noexcept;
	void CreateBuffers(StagingBufferManager& stagingBufferMan);
	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t meshBufferBindingSlot
	) const noexcept;

	void Draw(const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout) const noexcept;
	void Draw(
		const VKCommandBuffer& graphicsBuffer, const PipelineLayout& pipelineLayout
	) const noexcept {
		Draw(graphicsBuffer, pipelineLayout.Get());
	}

	void CleanupTempData() noexcept;

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		// One for the modelBufferIndex and another for the meshletOffset.
		return static_cast<std::uint32_t>(sizeof(std::uint32_t) * 2u);
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
	Buffer                    m_meshletBuffer;
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
		m_meshletBuffer{ std::move(other.m_meshletBuffer) },
		m_meshlets{ std::move(other.m_meshlets) }
	{}
	ModelBundleMS& operator=(ModelBundleMS&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelDetails       = std::move(other.m_modelDetails);
		m_meshletBuffer      = std::move(other.m_meshletBuffer);
		m_meshlets           = std::move(other.m_meshlets);

		return *this;
	}
};

class ModelBundleCSIndirect
{
public:
	ModelBundleCSIndirect();

	void AddModelDetails(const std::shared_ptr<ModelVS>& model) noexcept;
	void CreateBuffers(
		StagingBufferManager& stagingBufferMan, SharedBuffer& argumentInputSharedData,
		SharedBuffer& cullingSharedData
	);

	void Dispatch(const VKCommandBuffer& computeBuffer) const noexcept;

	void CleanupTempData() noexcept;

	void SetID(std::uint32_t bundleID) noexcept { m_bundleID = bundleID; }

	[[nodiscard]]
	std::uint32_t GetID() const noexcept { return m_bundleID; }

	[[nodiscard]]
	SharedBufferData GetArgumentInputSharedData() const noexcept { return m_argumentInputSharedData; }
	[[nodiscard]]
	SharedBufferData GetCullingSharedData() const noexcept { return m_cullingSharedData; }

private:
	struct CullingData
	{
		std::uint32_t     commandCount;
		std::uint32_t     padding;	// Next Vec2 starts at 8bytes offset
		DirectX::XMFLOAT2 xBounds;
		DirectX::XMFLOAT2 yBounds;
		DirectX::XMFLOAT2 zBounds;
	};

private:
	SharedBufferData                          m_argumentInputSharedData;
	SharedBufferData                          m_cullingSharedData;
	std::vector<VkDrawIndexedIndirectCommand> m_indirectArguments;
	std::unique_ptr<CullingData>              m_cullingData;
	std::uint32_t                             m_dispatchXCount;
	std::uint32_t                             m_bundleID;

	static constexpr DirectX::XMFLOAT2 XBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 YBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 ZBOUNDS = { 1.f, -1.f };

	// Each Compute Thread Group should have 64 threads.
	static constexpr float THREADBLOCKSIZE     = 64.f;

public:
	ModelBundleCSIndirect(const ModelBundleCSIndirect&) = delete;
	ModelBundleCSIndirect& operator=(const ModelBundleCSIndirect&) = delete;

	ModelBundleCSIndirect(ModelBundleCSIndirect&& other) noexcept
		: m_argumentInputSharedData{ other.m_argumentInputSharedData },
		m_cullingSharedData{ std::move(other.m_cullingSharedData) },
		m_indirectArguments{ std::move(other.m_indirectArguments) },
		m_cullingData{ std::move(other.m_cullingData) },
		m_dispatchXCount{ other.m_dispatchXCount },
		m_bundleID{ other.m_bundleID }
	{}
	ModelBundleCSIndirect& operator=(ModelBundleCSIndirect&& other) noexcept
	{
		m_argumentInputSharedData = other.m_argumentInputSharedData;
		m_cullingSharedData       = std::move(other.m_cullingSharedData);
		m_indirectArguments       = std::move(other.m_indirectArguments);
		m_cullingData             = std::move(other.m_cullingData);
		m_dispatchXCount          = other.m_dispatchXCount;
		m_bundleID                = other.m_bundleID;

		return *this;
	}
};

class ModelBundleVSIndirect : public ModelBundle
{
public:
	ModelBundleVSIndirect(
		VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices
	);

	void AddModelDetails(std::uint32_t modelBufferIndex) noexcept;

	void CreateBuffers(std::uint32_t frameCount, StagingBufferManager& stagingBufferMan);
	void Draw(const VKCommandBuffer& graphicsBuffer, VkDeviceSize frameIndex) const noexcept;

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex,
		std::uint32_t argumentsBindingSlot, std::uint32_t counterBindingSlot,
		std::uint32_t modelIndicesBindingSlot
	) const noexcept;

	void ResetCounterBuffer(VKCommandBuffer& transferBuffer, VkDeviceSize frameIndex) const noexcept;

	void CleanupTempData() noexcept;

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

private:
	std::uint32_t                  m_modelCount;
	QueueIndices3                  m_queueIndices;
	VkDeviceSize                   m_argumentBufferSize;
	VkDeviceSize                   m_counterBufferSize;
	Buffer                         m_argumentBuffer;
	Buffer                         m_counterBuffer;
	Buffer                         m_counterResetBuffer;
	Buffer                         m_modelIndicesBuffer;
	// I am gonna use the DrawIndex in the Vertex shader and the thread Index in the Compute shader
	// to index into this buffer and that will give us the actual model index.
	std::vector<std::uint32_t>     m_modelIndices;
	std::unique_ptr<std::uint32_t> m_counterResetData;

public:
	ModelBundleVSIndirect(const ModelBundleVSIndirect&) = delete;
	ModelBundleVSIndirect& operator=(const ModelBundleVSIndirect&) = delete;

	ModelBundleVSIndirect(ModelBundleVSIndirect&& other) noexcept
		: ModelBundle{ std::move(other) }, m_modelCount{ other.m_modelCount },
		m_queueIndices{ other.m_queueIndices },
		m_argumentBufferSize{ other.m_argumentBufferSize },
		m_counterBufferSize{ other.m_counterBufferSize },
		m_argumentBuffer{ std::move(other.m_argumentBuffer) },
		m_counterBuffer{ std::move(other.m_counterBuffer) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_modelIndicesBuffer{ std::move(other.m_modelIndicesBuffer) },
		m_modelIndices{ std::move(other.m_modelIndices) },
		m_counterResetData{ std::move(other.m_counterResetData) }
	{}
	ModelBundleVSIndirect& operator=(ModelBundleVSIndirect&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelCount         = other.m_modelCount;
		m_queueIndices       = other.m_queueIndices;
		m_argumentBufferSize = other.m_argumentBufferSize;
		m_counterBufferSize  = other.m_counterBufferSize;
		m_argumentBuffer     = std::move(other.m_argumentBuffer);
		m_counterBuffer      = std::move(other.m_counterBuffer);
		m_counterResetBuffer = std::move(other.m_counterResetBuffer);
		m_modelIndicesBuffer = std::move(other.m_modelIndicesBuffer);
		m_modelIndices       = std::move(other.m_modelIndices);
		m_counterResetData   = std::move(other.m_counterResetData);

		return *this;
	}
};

class ModelBuffers : public ReusableVkBuffer<ModelBuffers, std::shared_ptr<Model>>
{
	friend class ReusableVkBuffer<ModelBuffers, std::shared_ptr<Model>>;
public:
	ModelBuffers(VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount)
		: ReusableVkBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_modelBuffersInstanceSize{ 0u }, m_bufferInstanceCount{ frameCount }
	{}

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot
	) const noexcept;

	void Update(VkDeviceSize bufferIndex) const noexcept;

	[[nodiscard]]
	std::uint32_t GetInstanceCount() const noexcept
	{
		return m_bufferInstanceCount;
	}

private:
	struct ModelData
	{
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMFLOAT3 modelOffset;
		float             padding;
		// GLSL vec3 is actually vec4.
		std::uint32_t     materialIndex;
	};

private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(ModelData); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t modelCount);

	void _remove(size_t index) noexcept;

private:
	VkDeviceSize  m_modelBuffersInstanceSize;
	std::uint32_t m_bufferInstanceCount;

public:
	ModelBuffers(const ModelBuffers&) = delete;
	ModelBuffers& operator=(const ModelBuffers&) = delete;

	ModelBuffers(ModelBuffers&& other) noexcept
		: ReusableVkBuffer{ std::move(other) },
		m_modelBuffersInstanceSize{ other.m_modelBuffersInstanceSize },
		m_bufferInstanceCount{ other.m_bufferInstanceCount }
	{}
	ModelBuffers& operator=(ModelBuffers&& other) noexcept
	{
		ReusableVkBuffer::operator=(std::move(other));
		m_modelBuffersInstanceSize = other.m_modelBuffersInstanceSize;
		m_bufferInstanceCount      = other.m_bufferInstanceCount;

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
	bool  TempData
>
class ModelManager
{
public:
	ModelManager(VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_pipelineLayout{ device }, m_renderPass{ nullptr }, m_shaderPath{},
		m_modelBuffers{ device, memoryManager, frameCount },
		m_meshBoundsBuffer{ device, memoryManager }, m_graphicsPipelines{}, m_modelBundles{}
	{}

	void CreatePipelineLayout(const VkDescriptorBuffer& descriptorBuffer)
	{
		static_cast<Derived*>(this)->CreatePipelineLayoutImpl(descriptorBuffer);
	}

	void UpdatePerFrame(VkDeviceSize frameIndex) const noexcept
	{
		m_modelBuffers.Update(frameIndex);
	}

	void UpdateWhenGPUFinished() noexcept
	{
		m_meshBoundsBuffer.Update();
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
	std::uint32_t AddModel(std::shared_ptr<ModelType>&& model, const std::wstring& fragmentShader)
	{
		// This is necessary since the model buffers needs an Rvalue ref and returns the modelIndex,
		// which is necessary to add MeshDetails. Which can't be done without the modelIndex.
		std::shared_ptr<ModelType> tempModel = model;
		const std::uint32_t meshIndex        = model->GetMeshIndex();

		const size_t modelIndex = m_modelBuffers.Add(std::move(model));

		ModelBundleType modelBundle{};
		static_cast<Derived*>(this)->ConfigureModel(modelBundle, modelIndex, tempModel);

		modelBundle.SetMeshIndex(meshIndex);

		const std::uint32_t psoIndex = GetPSOIndex(fragmentShader);

		modelBundle.SetPSOIndex(psoIndex);

		const auto bundleID = static_cast<std::uint32_t>(modelBundle.GetID());

		AddModelBundle(std::move(modelBundle), psoIndex);

		return bundleID;
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelType>>&& modelBundle, const std::wstring& fragmentShader
	) {
		const size_t modelCount = std::size(modelBundle);

		if (modelCount)
		{
			// All of the models in a bundle should have the same mesh.
			const std::uint32_t meshIndex = modelBundle.front()->GetMeshIndex();

			std::vector<std::shared_ptr<Model>> tempModelBundle{ modelCount, nullptr };

			for (size_t index = 0u; index < modelCount; ++index)
				tempModelBundle.at(index) = std::static_pointer_cast<Model>(modelBundle.at(index));

			const std::vector<size_t> modelIndices
				= m_modelBuffers.AddMultiple(std::move(tempModelBundle));

			ModelBundleType modelBundleObj{};

			static_cast<Derived*>(this)->ConfigureModelBundle(
				modelBundleObj, modelIndices, modelBundle
			);

			modelBundleObj.SetMeshIndex(meshIndex);

			const std::uint32_t psoIndex = GetPSOIndex(fragmentShader);

			modelBundleObj.SetPSOIndex(psoIndex);

			const auto bundleID = static_cast<std::uint32_t>(modelBundle.GetID());

			AddModelBundle(std::move(modelBundleObj), psoIndex);

			return bundleID;
		}

		return std::numeric_limits<std::uint32_t>::max();
	}

	void RemoveBundle(std::uint32_t bundleID) noexcept
	{
		auto result = std::ranges::find(
			m_modelBundles, bundleID, {},
			[](const ModelBundleType& bundle) { return bundle.GetID(); }
		);

		if (result != std::end(m_modelBundles))
		{
			const auto modelBundleIndex = static_cast<size_t>(
				std::distance(std::begin(m_modelBundles), result)
			);

			static_cast<Derived*>(this)->ConfigureRemove(modelBundleIndex);

			m_modelBundles.erase(result);
		}
	}

	// Need a function which can change the PSO index of a Bundle.

	void AddMeshBundle(
		std::unique_ptr<MeshType> meshBundle, StagingBufferManager& stagingBufferMan
	) {
		MeshManager meshManager{ m_device, m_memoryManager };

		meshManager.SetMeshBundle(std::move(meshBundle), stagingBufferMan);

		m_meshBundles.emplace_back(std::move(meshManager));
	}

	void CleanUpTempData() noexcept
	{
		for (auto& meshBundle : m_meshBundles)
			meshBundle.CleanupTempData();

		if constexpr (TempData)
			static_cast<Derived*>(this)->_cleanUpTempData();
	}

protected:
	[[nodiscard]]
	std::optional<std::uint32_t> TryToGetPSOIndex(const std::wstring& fragmentShader) const noexcept
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

	[[nodiscard]]
	// Adds a new PSO, if one can't be found.
	std::uint32_t GetPSOIndex(const std::wstring& fragmentShader) noexcept
	{
		std::uint32_t psoIndex = 0u;

		auto oPSOIndex = TryToGetPSOIndex(fragmentShader);

		if (!oPSOIndex)
		{
			psoIndex = static_cast<std::uint32_t>(std::size(m_graphicsPipelines));

			Pipeline pipeline{};
			pipeline.Create(m_device, m_pipelineLayout, *m_renderPass, m_shaderPath, fragmentShader);

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

private:
	void AddModelBundle(ModelBundleType&& modelBundle, std::uint32_t psoIndex) noexcept
	{
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
	PipelineLayout               m_pipelineLayout;
	VKRenderPass*                m_renderPass;
	std::wstring                 m_shaderPath;
	ModelBuffers                 m_modelBuffers;
	// The members of MeshBounds should be implemented on the child which wants use it.
	MeshBoundsBuffers            m_meshBoundsBuffer;

	std::vector<Pipeline>        m_graphicsPipelines;
	std::vector<MeshManager>     m_meshBundles;
	std::vector<ModelBundleType> m_modelBundles;

	// Need to update this when I update the shaders.
	static constexpr std::uint32_t s_modelBuffersGraphicsBindingSlot = 0u;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ other.m_memoryManager },
		m_pipelineLayout{ std::move(other.m_pipelineLayout) },
		m_renderPass{ other.m_renderPass },
		m_shaderPath{ std::move(other.m_shaderPath) },
		m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_meshBoundsBuffer{ std::move(other.m_meshBoundsBuffer) },
		m_graphicsPipelines{ std::move(other.m_graphicsPipelines) },
		m_meshBundles{ std::move(other.m_meshBundles) },
		m_modelBundles{ std::move(other.m_modelBundles) }
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_device            = other.m_device;
		m_memoryManager     = other.m_memoryManager;
		m_pipelineLayout    = std::move(other.m_pipelineLayout);
		m_renderPass        = other.m_renderPass;
		m_shaderPath        = std::move(other.m_shaderPath);
		m_modelBuffers      = std::move(other.m_modelBuffers);
		m_meshBoundsBuffer  = std::move(other.m_meshBoundsBuffer);
		m_graphicsPipelines = std::move(other.m_graphicsPipelines);
		m_meshBundles       = std::move(other.m_meshBundles);
		m_modelBundles      = std::move(other.m_modelBundles);

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
		false
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndividual,
			GraphicsPipelineIndividualDraw,
			MeshManagerVertexShader, MeshBundleVS,
			ModelBundleVSIndividual, ModelVS,
			false
		>;
	friend class ModelManagerVSIndividualTest;
public:
	ModelManagerVSIndividual(VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount)
		: ModelManager{ device, memoryManager, frameCount }
	{}

	void SetDescriptorBufferLayout(std::vector<VkDescriptorBuffer>& descriptorBuffers);
	void SetDescriptorBuffer(std::vector<VkDescriptorBuffer>& descriptorBuffers);

	void Draw(const VKCommandBuffer& graphicsBuffer) const noexcept;

private:
	void CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer);

	void ConfigureModel(
		ModelBundleVSIndividual& modelBundleObj,
		size_t modelIndex, const std::shared_ptr<ModelVS>& model
	);
	void ConfigureModelBundle(
		ModelBundleVSIndividual& modelBundleObj,
		const std::vector<size_t>& modelIndices,
		const std::vector<std::shared_ptr<ModelVS>>& modelBundle
	);

	void ConfigureRemove(size_t bundleIndex) noexcept;

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

class ModelManagerVSIndirect : public
	ModelManager
	<
		ModelManagerVSIndirect,
		GraphicsPipelineIndirectDraw,
		MeshManagerVertexShader, MeshBundleVS,
		ModelBundleVSIndirect, ModelVS,
		true
	>
{
	friend class ModelManager
		<
			ModelManagerVSIndirect,
			GraphicsPipelineIndirectDraw,
			MeshManagerVertexShader, MeshBundleVS,
			ModelBundleVSIndirect, ModelVS,
			true
		>;
	friend class ModelManagerVSIndirectTest;
public:
	ModelManagerVSIndirect(
		VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
		std::uint32_t frameCount
	);

	void CreateBuffers(StagingBufferManager& stagingBufferMan);
	void CreatePipelineCS(const VkDescriptorBuffer& descriptorBuffer);

	// Need a remove method and a CopyTempBuffer method.

	void SetDescriptorBufferLayoutVS(std::vector<VkDescriptorBuffer>& descriptorBuffers);
	void SetDescriptorBufferVS(std::vector<VkDescriptorBuffer>& descriptorBuffers);

	void SetDescriptorBufferLayoutCS(std::vector<VkDescriptorBuffer>& descriptorBuffers);
	void SetDescriptorBufferCS(std::vector<VkDescriptorBuffer>& descriptorBuffers);

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

	void ConfigureRemove(size_t bundleIndex) noexcept;

	void _cleanUpTempData() noexcept;

private:
	StagingBufferManager*              m_stagingBufferMan;
	SharedBuffer                       m_argumentInputBuffer;
	SharedBuffer                       m_cullingDataBuffer;
	PipelineLayout                     m_pipelineLayoutCS;
	ComputePipeline                    m_computePipeline;
	// The indices of the CS bundles won't align with the VS ones, but that shouldn't be an issue
	// as we should be setting the correct offset in the Configure method.

	// Now that I think about it, it might be better to just have a single CS bundle. But that will
	// have the downside of managing the data for each bundle and adding new ones.
	std::vector<ModelBundleCSIndirect> m_modelBundlesCS;

	// Need to update these when I update the shaders.
	static constexpr std::uint32_t s_modelIndicesVSBindingSlot      = 1u;
	static constexpr std::uint32_t s_modelIndicesCSBindingSlot      = 1u;
	static constexpr std::uint32_t s_modelBuffersComputeBindingSlot = 0u;
	static constexpr std::uint32_t s_argumentInputBufferBindingSlot = 2u;
	static constexpr std::uint32_t s_cullingDataBufferBindingSlot   = 2u;

public:
	ModelManagerVSIndirect(const ModelManagerVSIndirect&) = delete;
	ModelManagerVSIndirect& operator=(const ModelManagerVSIndirect&) = delete;

	ModelManagerVSIndirect(ModelManagerVSIndirect&& other) noexcept
		: ModelManager{ std::move(other) },
		m_stagingBufferMan{ other.m_stagingBufferMan },
		m_argumentInputBuffer{ std::move(other.m_argumentInputBuffer) },
		m_cullingDataBuffer{ std::move(other.m_cullingDataBuffer) },
		m_pipelineLayoutCS{ std::move(other.m_pipelineLayoutCS) },
		m_computePipeline{ std::move(other.m_computePipeline) },
		m_modelBundlesCS{ std::move(other.m_modelBundlesCS) }
	{}
	ModelManagerVSIndirect& operator=(ModelManagerVSIndirect&& other) noexcept
	{
		ModelManager::operator=(std::move(other));
		m_stagingBufferMan    = other.m_stagingBufferMan;
		m_argumentInputBuffer = std::move(other.m_argumentInputBuffer);
		m_cullingDataBuffer   = std::move(other.m_cullingDataBuffer);
		m_pipelineLayoutCS    = std::move(other.m_pipelineLayoutCS);
		m_computePipeline     = std::move(other.m_computePipeline);
		m_modelBundlesCS      = std::move(other.m_modelBundlesCS);

		return *this;
	}
};
#endif
