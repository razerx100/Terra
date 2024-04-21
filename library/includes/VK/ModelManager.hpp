#ifndef MODEL_MANAGER_HPP_
#define MODEL_MANAGER_HPP_
#include <VkResources.hpp>
#include <VkCommandQueue.hpp>
#include <VkExtensionManager.hpp>
#include <VkQueueFamilyManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <memory>

#include <IModel.hpp>

// The idea behind this class would be to either have a single model in it or multiple of them.
// If there are multiple models in it, all of their vertices will be packed into a single buffer.
// And you will need to get rid of the whole bundle if you just want to get rid of one model from it.
// I am doing this, since adding and removing elements from gvertices would be a pain and probably not
// worth it. But with this way, I can have gvertices support and also that should help when doing mesh
// shading. Mesh shading for a single small model wouldn't really be that efficient.
class ModelBundle
{
public:
	// I think I might be able to store the Argument Buffer and the counter buffer here. And from
	// the child class' function, bind them to the compute descriptor. And that should be it? I need
	// to keep them here, since the Draw call needs them. But the compute queue just needs to output
	// to them. I shouldn't need to do frustum culling for all models in one dispatch call, as long
	// as they are in the same submission, it should be the same? The same should be applicable to
	// the meshlets as well.
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
	void Draw(VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout) const noexcept;

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
	ModelBundleMeshShader() : ModelBundle{}, m_modelDetails{} {}

	void AddModel(std::uint32_t meshletCount, std::uint32_t modelIndex) noexcept;
	void Draw(VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout) const noexcept;

private:
	struct ModelDetails
	{
		std::uint32_t modelIndex;
		std::uint32_t meshletCount;
	};

private:
	std::vector<ModelDetails> m_modelDetails;

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
		: ModelBundle{ std::move(other) }, m_modelDetails{ std::move(other.m_modelDetails) }
	{}
	ModelBundleMeshShader& operator=(ModelBundleMeshShader&& other) noexcept
	{
		ModelBundle::operator=(std::move(other));
		m_modelDetails       = std::move(other.m_modelDetails);

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

	void Dispatch(VKCommandBuffer& computeBuffer) const noexcept;

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
		VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount,
		QueueIndices3 queueIndices
	);

	void CreateBuffers(std::uint32_t modelCount, StagingBufferManager& stagingBufferMan);
	void Draw(VKCommandBuffer& graphicsBuffer, VkDeviceSize frameIndex) const noexcept;

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
	std::uint32_t                  m_frameCount;

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
		m_counterResetData{ std::move(other.m_counterResetData) },
		m_frameCount{ other.m_frameCount }
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
		m_frameCount         = other.m_frameCount;

		return *this;
	}
};

// I will put a VertexManager, ModelBundles and PSOs in this class. Multiple modelBundles can use a
// single modelManager and a single PSO. And can also change them.
// I want to keep the bare minimum here to draw models. I want to add deferred rendering later,
// while I think I can reuse ModelBundle, I feel like won't be able to reuse ModelManager.
class ModelManager
{
public:
	ModelManager() {}

private:
public:
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	ModelManager(ModelManager&& other) noexcept
	{}
	ModelManager& operator=(ModelManager&& other) noexcept
	{
		return *this;
	}
};
#endif
