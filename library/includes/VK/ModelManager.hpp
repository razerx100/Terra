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

#include <VkModelBundle.hpp>
#include <CommonBuffers.hpp>
#include <Shader.hpp>

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
		m_graphicsPipelines{}, m_meshBundles{}, m_modelBundles{}, m_oldBufferCopyNecessary{ false }
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

			m_oldBufferCopyNecessary = true;

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

		auto meshIndex           = m_meshBundles.Add(std::move(meshManager));

		m_oldBufferCopyNecessary = true;

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
	bool                         m_oldBufferCopyNecessary;

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
		m_oldBufferCopyNecessary{ other.m_oldBufferCopyNecessary }
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
		m_oldBufferCopyNecessary = other.m_oldBufferCopyNecessary;

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

	void CopyOldBuffers(const VKCommandBuffer& transferCmdBuffer) noexcept;

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

	void CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept;

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

	void CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept;

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
