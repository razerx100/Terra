#ifndef VK_MODEL_BUFFER_HPP_
#define VK_MODEL_BUFFER_HPP_
#include <memory>
#include <ModelContainer.hpp>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>

namespace Terra
{
class ModelBuffers
{
	using ModelContainer_t = std::shared_ptr<ModelContainer>;

public:
	ModelBuffers(
		VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount,
		const std::vector<std::uint32_t>& modelBufferQueueIndices
	) : m_modelContainer{},
		m_vertexModelBuffers{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_fragmentModelBuffers{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_modelBuffersInstanceSize{ 0u }, m_modelBuffersFragmentInstanceSize{ 0u },
		m_bufferInstanceCount{ frameCount }, m_modelBuffersQueueIndices{ modelBufferQueueIndices }
	{}

	void SetModelContainer(std::shared_ptr<ModelContainer> modelContainer) noexcept
	{
		m_modelContainer = std::move(modelContainer);
	}

	void ExtendModelBuffers();

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
		DirectX::XMMATRIX normalMatrix;
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
	ModelContainer_t           m_modelContainer;
	Buffer                     m_vertexModelBuffers;
	Buffer                     m_fragmentModelBuffers;
	VkDeviceSize               m_modelBuffersInstanceSize;
	VkDeviceSize               m_modelBuffersFragmentInstanceSize;
	std::uint32_t              m_bufferInstanceCount;
	std::vector<std::uint32_t> m_modelBuffersQueueIndices;

public:
	ModelBuffers(const ModelBuffers&) = delete;
	ModelBuffers& operator=(const ModelBuffers&) = delete;

	ModelBuffers(ModelBuffers&& other) noexcept
		: m_modelContainer{ std::move(other.m_modelContainer) },
		m_vertexModelBuffers{ std::move(other.m_vertexModelBuffers) },
		m_fragmentModelBuffers{ std::move(other.m_fragmentModelBuffers) },
		m_modelBuffersInstanceSize{ other.m_modelBuffersInstanceSize },
		m_modelBuffersFragmentInstanceSize{ other.m_modelBuffersFragmentInstanceSize },
		m_bufferInstanceCount{ other.m_bufferInstanceCount },
		m_modelBuffersQueueIndices{ std::move(other.m_modelBuffersQueueIndices) }
	{}
	ModelBuffers& operator=(ModelBuffers&& other) noexcept
	{
		m_modelContainer                   = std::move(other.m_modelContainer);
		m_vertexModelBuffers               = std::move(other.m_vertexModelBuffers);
		m_fragmentModelBuffers             = std::move(other.m_fragmentModelBuffers);
		m_modelBuffersInstanceSize         = other.m_modelBuffersInstanceSize;
		m_modelBuffersFragmentInstanceSize = other.m_modelBuffersFragmentInstanceSize;
		m_bufferInstanceCount              = other.m_bufferInstanceCount;
		m_modelBuffersQueueIndices         = std::move(other.m_modelBuffersQueueIndices);

		return *this;
	}
};
}
#endif
