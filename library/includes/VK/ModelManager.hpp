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
#include <ReusableVkBuffer.hpp>

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
	ModelBundleVSIndividual() : ModelBundle{}, m_meshDetails{} {}

	void AddMeshDetails(const std::shared_ptr<ModelVS>& model, std::uint32_t modelBufferIndex) noexcept;
	void Draw(const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout) const noexcept;

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(std::uint32_t));
	}

private:
	struct MeshDetails
	{
		std::uint32_t                modelBufferIndex;
		VkDrawIndexedIndirectCommand indexedArguments;
	};

private:
	std::vector<MeshDetails> m_meshDetails;

public:
	ModelBundleVSIndividual(const ModelBundleVSIndividual&) = delete;
	ModelBundleVSIndividual& operator=(const ModelBundleVSIndividual&) = delete;

	ModelBundleVSIndividual(ModelBundleVSIndividual&& other) noexcept
		: ModelBundle{ std::move(other) }, m_meshDetails{ std::move(other.m_meshDetails) }
	{}
	ModelBundleVSIndividual& operator=(ModelBundleVSIndividual&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_meshDetails        = std::move(other.m_meshDetails);

		return *this;
	}
};

class ModelBundleMS : public ModelBundle
{
public:
	ModelBundleMS(VkDevice device, MemoryManager* memoryManager)
		: ModelBundle{}, m_meshDetails{},
		m_meshletBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
		m_meshlets{}
	{}

	// Need this one as non-const, since I am gonna move the meshlets.
	void AddMeshDetails(std::shared_ptr<ModelMS>& model, std::uint32_t modelBufferIndex) noexcept;
	void CreateBuffers(StagingBufferManager& stagingBufferMan);
	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t meshBufferBindingSlot
	) const noexcept;

	void Draw(const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout) const noexcept;

	void CleanupTempData() noexcept;

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		// One for the modelBufferIndex and another for the meshletOffset.
		return static_cast<std::uint32_t>(sizeof(std::uint32_t) * 2u);
	}

private:
	struct MeshDetails
	{
		std::uint32_t modelBufferIndex;
		// Since there could be meshlets of multiple Models, we need the starting point of each.
		std::uint32_t meshletOffset;
		std::uint32_t threadGroupCountX;
	};

private:
	std::vector<MeshDetails> m_meshDetails;
	Buffer                   m_meshletBuffer;
	std::vector<Meshlet>     m_meshlets;

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
		m_meshDetails{ std::move(other.m_meshDetails) },
		m_meshletBuffer{ std::move(other.m_meshletBuffer) },
		m_meshlets{ std::move(other.m_meshlets) }
	{}
	ModelBundleMS& operator=(ModelBundleMS&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_meshDetails        = std::move(other.m_meshDetails);
		m_meshletBuffer      = std::move(other.m_meshletBuffer);
		m_meshlets           = std::move(other.m_meshlets);

		return *this;
	}
};

class ModelBundleCSIndirect
{
public:
	ModelBundleCSIndirect(VkDevice device, MemoryManager* memoryManager);

	void AddMeshDetails(const std::shared_ptr<ModelVS>& model, std::uint32_t modelBufferIndex) noexcept;
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
		std::uint32_t                modelBufferIndex;
	// This object has 5, 32bits int. So, I can put another without adding any implicit paddings.
		VkDrawIndexedIndirectCommand indirectArguments;
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

	// Each Compute Thread Group should have 64 threads.
	static constexpr float THREADBLOCKSIZE     = 64.f;

public:
	ModelBundleCSIndirect(const ModelBundleCSIndirect&) = delete;
	ModelBundleCSIndirect& operator=(const ModelBundleCSIndirect&) = delete;

	ModelBundleCSIndirect(ModelBundleCSIndirect&& other) noexcept
		: m_argumentInputBuffer{ std::move(other.m_argumentInputBuffer) },
		m_cullingDataBuffer{ std::move(other.m_cullingDataBuffer) },
		m_indirectArguments{ std::move(other.m_indirectArguments) },
		m_cullingData{ std::move(other.m_cullingData) },
		m_dispatchXCount{ other.m_dispatchXCount }
	{}
	ModelBundleCSIndirect& operator=(ModelBundleCSIndirect&& other) noexcept
	{
		m_argumentInputBuffer = std::move(other.m_argumentInputBuffer);
		m_cullingDataBuffer   = std::move(other.m_cullingDataBuffer);
		m_indirectArguments   = std::move(other.m_indirectArguments);
		m_cullingData         = std::move(other.m_cullingData);
		m_dispatchXCount      = other.m_dispatchXCount;

		return *this;
	}
};

class ModelBundleVSIndirect : public ModelBundle
{
public:
	ModelBundleVSIndirect(
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

// I will put a MeshManager, ModelBundles and PSOs in this class. Multiple ModelBundles can use a
// single MeshManager and a single PSO. And can also change them.
// I want to keep the bare minimum here to draw models. I want to add deferred rendering later,
// while I think I can reuse ModelBundle, I feel like won't be able to reuse ModelManager.
template<class Derived>
class ModelManager
{
public:
	ModelManager(VkDevice device) : m_pipelineLayout{ device } {}

	void CreatePipelineLayout(const VkDescriptorBuffer& descriptorBuffer)
	{
		static_cast<Derived*>(this)->CreatePipelineLayoutImpl(descriptorBuffer);
	}

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

class ModelManagerVSIndividual : public ModelManager<ModelManagerVSIndividual>
{
	friend class ModelManager<ModelManagerVSIndividual>;
public:
	ModelManagerVSIndividual(VkDevice device) : ModelManager{ device }
	{}

private:
	void CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer);

private:


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
#endif
