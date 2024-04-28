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

#include <IModel.hpp>

class ModelBundle
{
public:
	ModelBundle() : m_psoIndex{ 0u }, m_vertexManagerIndex{ 0u }, m_modelID{ 0u } {}

	void SetPSOIndex(std::uint32_t index) noexcept { m_psoIndex = index; }
	void SetVertexManagerIndex(std::uint32_t index) noexcept { m_vertexManagerIndex = index; }

	[[nodiscard]]
	std::uint32_t GetPSOIndex() const noexcept { return m_psoIndex; }
	[[nodiscard]]
	std::uint32_t GetVertexManagerIndex() const noexcept { return m_vertexManagerIndex; }

protected:
	std::uint32_t m_psoIndex;
	std::uint32_t m_vertexManagerIndex;
	size_t        m_modelID;

public:
	ModelBundle(const ModelBundle&) = delete;
	ModelBundle& operator=(const ModelBundle&) = delete;

	ModelBundle(ModelBundle&& other) noexcept
		: m_psoIndex{ other.m_psoIndex }, m_vertexManagerIndex{ other.m_vertexManagerIndex },
		m_modelID{ other.m_modelID }
	{}
	ModelBundle& operator=(ModelBundle&& other) noexcept
	{
		m_psoIndex           = other.m_psoIndex;
		m_vertexManagerIndex = other.m_vertexManagerIndex;
		m_modelID            = other.m_modelID;

		return *this;
	}
};

class ModelBundleVertexShaderIndividual : public ModelBundle
{
public:
	ModelBundleVertexShaderIndividual() : ModelBundle{}, m_modelDetails{} {}

	void AddModel(
		const VkDrawIndexedIndirectCommand& drawArguments, std::uint32_t modelIndex
	) noexcept;
	void Draw(const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout) const noexcept;

private:
	struct ModelDetails
	{
		std::uint32_t                modelIndex;
		VkDrawIndexedIndirectCommand indexedArguments;
	};

private:
	std::vector<ModelDetails> m_modelDetails;

public:
	ModelBundleVertexShaderIndividual(const ModelBundleVertexShaderIndividual&) = delete;
	ModelBundleVertexShaderIndividual& operator=(const ModelBundleVertexShaderIndividual&) = delete;

	ModelBundleVertexShaderIndividual(ModelBundleVertexShaderIndividual&& other) noexcept
		: ModelBundle{ std::move(other) }, m_modelDetails{ std::move(other.m_modelDetails) }
	{}
	ModelBundleVertexShaderIndividual& operator=(ModelBundleVertexShaderIndividual&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelDetails       = std::move(other.m_modelDetails);

		return *this;
	}
};

class ModelBundleMeshShader : public ModelBundle
{
public:
	ModelBundleMeshShader(VkDevice device, MemoryManager* memoryManager)
		: ModelBundle{}, m_modelDetails{},
		m_meshletBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
		m_meshlets{}
	{}

	void AddModel(std::vector<Meshlet>&& meshlets, std::uint32_t modelIndex) noexcept;
	void CreateBuffers(StagingBufferManager& stagingBufferMan);
	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t meshBufferBindingSlot
	) const noexcept;

	void Draw(const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout) const noexcept;

	void CleanupTempData() noexcept;

private:
	struct ModelDetails
	{
		std::uint32_t modelIndex;
		std::uint32_t meshletOffset;
		std::uint32_t threadGroupCountX;
	};

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

	ModelBundleMeshShader(const ModelBundleMeshShader&) = delete;
	ModelBundleMeshShader& operator=(const ModelBundleMeshShader&) = delete;

	ModelBundleMeshShader(ModelBundleMeshShader&& other) noexcept
		: ModelBundle{ std::move(other) },
		m_modelDetails{ std::move(other.m_modelDetails) },
		m_meshletBuffer{ std::move(other.m_meshletBuffer) },
		m_meshlets{ std::move(other.m_meshlets) }
	{}
	ModelBundleMeshShader& operator=(ModelBundleMeshShader&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelDetails       = std::move(other.m_modelDetails);
		m_meshletBuffer      = std::move(other.m_meshletBuffer);
		m_meshlets           = std::move(other.m_meshlets);

		return *this;
	}
};

class ModelBundleComputeShaderIndirect
{
public:
	ModelBundleComputeShaderIndirect(VkDevice device, MemoryManager* memoryManager);

	void AddModel(
		const VkDrawIndexedIndirectCommand& drawArguments, std::uint32_t modelIndex
	) noexcept;
	void CreateBuffers(StagingBufferManager& stagingBufferMan);
	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer,
		std::uint32_t argumentInputBindingSlot, std::uint32_t cullingDataBindingSlot
	) const noexcept;

	void Dispatch(const VKCommandBuffer& computeBuffer) const noexcept;

	void CleanupTempData() noexcept;

public:
	struct Argument
	{
	// This object has 5, 32bits int. So, I can put another without adding any implicit paddings.
		VkDrawIndexedIndirectCommand indirectArguments;
		std::uint32_t                modelIndex;
	};

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
	Buffer                       m_argumentInputBuffer;
	Buffer                       m_cullingDataBuffer;
	std::vector<Argument>        m_indirectArguments;
	std::unique_ptr<CullingData> m_cullingData;
	std::uint32_t                m_dispatchXCount;

	static constexpr DirectX::XMFLOAT2 XBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 YBOUNDS = { 1.f, -1.f };
	static constexpr DirectX::XMFLOAT2 ZBOUNDS = { 1.f, -1.f };
	static constexpr float THREADBLOCKSIZE     = 64.f;

public:
	ModelBundleComputeShaderIndirect(const ModelBundleComputeShaderIndirect&) = delete;
	ModelBundleComputeShaderIndirect& operator=(const ModelBundleComputeShaderIndirect&) = delete;

	ModelBundleComputeShaderIndirect(ModelBundleComputeShaderIndirect&& other) noexcept
		: m_argumentInputBuffer{ std::move(other.m_argumentInputBuffer) },
		m_cullingDataBuffer{ std::move(other.m_cullingDataBuffer) },
		m_indirectArguments{ std::move(other.m_indirectArguments) },
		m_cullingData{ std::move(other.m_cullingData) },
		m_dispatchXCount{ other.m_dispatchXCount }
	{}
	ModelBundleComputeShaderIndirect& operator=(ModelBundleComputeShaderIndirect&& other) noexcept
	{
		m_argumentInputBuffer = std::move(other.m_argumentInputBuffer);
		m_cullingDataBuffer   = std::move(other.m_cullingDataBuffer);
		m_indirectArguments   = std::move(other.m_indirectArguments);
		m_cullingData         = std::move(other.m_cullingData);
		m_dispatchXCount      = other.m_dispatchXCount;

		return *this;
	}
};

class ModelBundleVertexShaderIndirect : public ModelBundle
{
public:
	ModelBundleVertexShaderIndirect(
		VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices
	);

	void CreateBuffers(
		std::uint32_t modelCount, std::uint32_t frameCount, StagingBufferManager& stagingBufferMan
	);
	void Draw(const VKCommandBuffer& graphicsBuffer, VkDeviceSize frameIndex) const noexcept;

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex,
		std::uint32_t argumentsBindingSlot, std::uint32_t counterBindingSlot
	) const noexcept;

	void ResetCounterBuffer(VKCommandBuffer& transferBuffer, VkDeviceSize frameIndex) const noexcept;

	void CleanupTempData() noexcept;

private:
	std::uint32_t                  m_modelCount;
	QueueIndices3                  m_queueIndices;
	VkDeviceSize                   m_argumentBufferSize;
	VkDeviceSize                   m_counterBufferSize;
	Buffer                         m_argumentBuffer;
	Buffer                         m_counterBuffer;
	Buffer                         m_counterResetBuffer;
	std::unique_ptr<std::uint32_t> m_counterResetData;

public:
	ModelBundleVertexShaderIndirect(const ModelBundleVertexShaderIndirect&) = delete;
	ModelBundleVertexShaderIndirect& operator=(const ModelBundleVertexShaderIndirect&) = delete;

	ModelBundleVertexShaderIndirect(ModelBundleVertexShaderIndirect&& other) noexcept
		: ModelBundle{ std::move(other) }, m_modelCount{ other.m_modelCount },
		m_queueIndices{ other.m_queueIndices },
		m_argumentBufferSize{ other.m_argumentBufferSize },
		m_counterBufferSize{ other.m_counterBufferSize },
		m_argumentBuffer{ std::move(other.m_argumentBuffer) },
		m_counterBuffer{ std::move(other.m_counterBuffer) },
		m_counterResetBuffer{ std::move(other.m_counterResetBuffer) },
		m_counterResetData{ std::move(other.m_counterResetData) }
	{}
	ModelBundleVertexShaderIndirect& operator=(ModelBundleVertexShaderIndirect&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelCount         = other.m_modelCount;
		m_queueIndices       = other.m_queueIndices;
		m_argumentBufferSize = other.m_argumentBufferSize;
		m_counterBufferSize  = other.m_counterBufferSize;
		m_argumentBuffer     = std::move(other.m_argumentBuffer);
		m_counterBuffer      = std::move(other.m_counterBuffer);
		m_counterResetBuffer = std::move(other.m_counterResetBuffer);
		m_counterResetData   = std::move(other.m_counterResetData);

		return *this;
	}
};

class ModelBuffers
{
public:
	ModelBuffers(VkDevice device, MemoryManager* memoryManager)
		: m_modelBuffers{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_modelBuffersInstanceSize{ 0u }, m_models{}
	{}

	void CreateBuffer(std::uint32_t frameCount);
	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot
	) const noexcept;

	void AddModel(std::shared_ptr<IModel>&& model) noexcept;
	void AddModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;

	void Update(VkDeviceSize bufferIndex) const;

private:
	struct ModelData
	{
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMFLOAT3 modelOffset;
		float             padding;
		// GLSL vec3 is actually vec4.
	};

private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(ModelData); }
	[[nodiscard]]
	size_t GetCount() const noexcept { return std::size(m_models); }

private:
	Buffer                               m_modelBuffers;
	VkDeviceSize                         m_modelBuffersInstanceSize;
	std::vector<std::shared_ptr<IModel>> m_models;

public:
	ModelBuffers(const ModelBuffers&) = delete;
	ModelBuffers& operator=(const ModelBuffers&) = delete;

	ModelBuffers(ModelBuffers&& other) noexcept
		: m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_modelBuffersInstanceSize{ other.m_modelBuffersInstanceSize },
		m_models{ std::move(other.m_models) }
	{}
	ModelBuffers& operator=(ModelBuffers&& other) noexcept
	{
		m_modelBuffers             = std::move(other.m_modelBuffers);
		m_modelBuffersInstanceSize = other.m_modelBuffersInstanceSize;
		m_models                   = std::move(other.m_models);

		return *this;
	}
};

// I will put a VertexManager, ModelBundles and PSOs in this class. Multiple modelBundles can use a
// single VertexManager and a single PSO. And can also change them.
// I want to keep the bare minimum here to draw models. I want to add deferred rendering later,
// while I think I can reuse ModelBundle, I feel like won't be able to reuse ModelManager.
class ModelManager
{
public:
	ModelManager(VkDevice device) : m_pipelineLayout{ device } {}

protected:
	PipelineLayout m_pipelineLayout;

public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
		: m_pipelineLayout{ std::move(other.m_pipelineLayout) }
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		m_pipelineLayout = std::move(other.m_pipelineLayout);
		return *this;
	}
};
#endif
