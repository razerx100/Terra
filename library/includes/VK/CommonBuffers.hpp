#ifndef COMMON_BUFFERS_HPP_
#define COMMON_BUFFERS_HPP_
#include <VkResources.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>

// Need something like an IMaterial. But for now will use IModel.
#include <IModel.hpp>

class MaterialBuffers
{
public:
	MaterialBuffers(VkDevice device, MemoryManager* memoryManager)
		: m_materialBuffers{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT }
	{}

	void CreateBuffer(StagingBufferManager& stagingBufferMan);
	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot
	) const noexcept;

	//void AddMaterial(std::shared_ptr<IMaterial>&& material) noexcept;
	//void AddMaterials(std::vector<std::shared_ptr<IMaterial>>&& materials) noexcept;

private:
	struct MaterialBuffer
	{
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 specular;
		UVInfo            diffuseTexUVInfo;
		UVInfo            specularTexUVInfo;
		std::uint32_t     diffuseTexIndex;
		std::uint32_t     specularTexIndex;
		float             shininess;
		float             padding;
	};

private:
	Buffer m_materialBuffers;

public:
	MaterialBuffers(const MaterialBuffers&) = delete;
	MaterialBuffers& operator=(const MaterialBuffers&) = delete;

	MaterialBuffers(MaterialBuffers&& other) noexcept
		: m_materialBuffers{ std::move(other.m_materialBuffers) }
	{}
	MaterialBuffers& operator=(MaterialBuffers&& other) noexcept
	{
		m_materialBuffers = std::move(other.m_materialBuffers);

		return *this;
	}
};

// For now, the BoundingBoxes seem to me like they should be linked to the Meshes not the Models.
class BoundingBoxBuffers
{
public:
	BoundingBoxBuffers(VkDevice device, MemoryManager* memoryManager)
		: m_boundingBoxBuffers{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT }
	{}

	void CreateBuffer(StagingBufferManager& stagingBufferMan);
	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot
	) const noexcept;

	void AddBoundingBox(ModelBounds boundingBox) noexcept;
	void AddBoundingBoxes(std::vector<ModelBounds>&& boundingBoxes) noexcept;

private:
	struct BoundingBox
	{
		DirectX::XMFLOAT3 positiveBounds;
		float padding0;
		DirectX::XMFLOAT3 negativeBounds;
		float padding1;
		// GLSL's vec3 is actually vec4.
	};

private:
	Buffer m_boundingBoxBuffers;

public:
	BoundingBoxBuffers(const BoundingBoxBuffers&) = delete;
	BoundingBoxBuffers& operator=(const BoundingBoxBuffers&) = delete;

	BoundingBoxBuffers(BoundingBoxBuffers&& other) noexcept
		: m_boundingBoxBuffers{ std::move(other.m_boundingBoxBuffers) }
	{}
	BoundingBoxBuffers& operator=(BoundingBoxBuffers&& other) noexcept
	{
		m_boundingBoxBuffers = std::move(other.m_boundingBoxBuffers);

		return *this;
	}
};
#endif
