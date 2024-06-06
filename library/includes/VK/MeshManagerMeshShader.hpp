#ifndef MESH_MANAGER_MESH_SHADER_HPP_
#define MESH_MANAGER_MESH_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <deque>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <CommonBuffers.hpp>

#include <MeshBundle.hpp>

class MeshManagerMeshShader
{
public:
	struct GLSLVertex
	{
		DirectX::XMFLOAT3 position;
		float padding0;
		DirectX::XMFLOAT3 normal;
		float padding1;
		DirectX::XMFLOAT2 uv;
		float padding3[2];
	};

	struct TempData
	{
		std::vector<GLSLVertex>       vertices;
		std::unique_ptr<MeshBundleMS> meshBundle;
	};

public:
	MeshManagerMeshShader(VkDevice device, MemoryManager* memoryManager);

	void SetMeshBundle(
		std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBuffer& vertexSharedBuffer,
		std::deque<TempData>& tempDataContainer
	);

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t verticesBindingSlot,
		std::uint32_t vertexIndicesBindingSlot, std::uint32_t primIndicesBindingSlot
	) const noexcept;

	[[nodiscard]]
	const std::vector<MeshBounds>& GetBounds() const noexcept { return m_meshBounds; }

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }

private:
	[[nodiscard]]
	static std::vector<GLSLVertex> TransformVertices(const std::vector<Vertex>& vertices) noexcept;

private:
	SharedBufferData        m_vertexBufferSharedData;
	Buffer                  m_vertexIndicesBuffer;
	Buffer                  m_primIndicesBuffer;
	std::vector<MeshBounds> m_meshBounds;

public:
	MeshManagerMeshShader(const MeshManagerMeshShader&) = delete;
	MeshManagerMeshShader& operator=(const MeshManagerMeshShader&) = delete;

	MeshManagerMeshShader(MeshManagerMeshShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_vertexIndicesBuffer{ std::move(other.m_vertexIndicesBuffer) },
		m_primIndicesBuffer{ std::move(other.m_primIndicesBuffer) },
		m_meshBounds{ std::move(other.m_meshBounds) }
	{}

	MeshManagerMeshShader& operator=(MeshManagerMeshShader&& other) noexcept
	{
		m_vertexBufferSharedData = std::move(other.m_vertexBufferSharedData);
		m_vertexIndicesBuffer    = std::move(other.m_vertexIndicesBuffer);
		m_primIndicesBuffer      = std::move(other.m_primIndicesBuffer);
		m_meshBounds             = std::move(other.m_meshBounds);

		return *this;
	}
};
#endif
