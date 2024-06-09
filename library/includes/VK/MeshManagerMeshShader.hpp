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

	// The offset should be enough to identify the mesh. We wouldn't really need the size
	// as the meshlets should already have that data.
	struct MeshDetails
	{
		std::uint32_t vertexOffset;
		std::uint32_t vertexIndicesOffset;
		std::uint32_t primIndicesOffset;
	};

	struct TempData
	{
		std::vector<GLSLVertex>       vertices;
		std::unique_ptr<MeshBundleMS> meshBundle;
	};

public:
	MeshManagerMeshShader();

	void SetMeshBundle(
		std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBuffer& vertexSharedBuffer, SharedBuffer& vertexIndicesSharedBuffer,
		SharedBuffer& primIndicesSharedBuffer, std::deque<TempData>& tempDataContainer
	);

	[[nodiscard]]
	const std::vector<MeshBound>& GetBounds() const noexcept { return m_meshBounds; }

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetVertexIndicesSharedData() const noexcept
	{
		return m_vertexIndicesBufferSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetPrimIndicesSharedData() const noexcept
	{
		return m_primIndicesBufferSharedData;
	}

	[[nodiscard]]
	MeshDetails GetMeshDetails() const noexcept { return m_meshDetails; }

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(MeshDetails));
	}

private:
	[[nodiscard]]
	static std::vector<GLSLVertex> TransformVertices(const std::vector<Vertex>& vertices) noexcept;

private:
	SharedBufferData       m_vertexBufferSharedData;
	SharedBufferData       m_vertexIndicesBufferSharedData;
	SharedBufferData       m_primIndicesBufferSharedData;
	std::vector<MeshBound> m_meshBounds;
	MeshDetails            m_meshDetails;

public:
	MeshManagerMeshShader(const MeshManagerMeshShader&) = delete;
	MeshManagerMeshShader& operator=(const MeshManagerMeshShader&) = delete;

	MeshManagerMeshShader(MeshManagerMeshShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_vertexIndicesBufferSharedData{ other.m_vertexIndicesBufferSharedData },
		m_primIndicesBufferSharedData{ other.m_primIndicesBufferSharedData },
		m_meshBounds{ std::move(other.m_meshBounds) },
		m_meshDetails{ other.m_meshDetails }
	{}

	MeshManagerMeshShader& operator=(MeshManagerMeshShader&& other) noexcept
	{
		m_vertexBufferSharedData        = other.m_vertexBufferSharedData;
		m_vertexIndicesBufferSharedData = other.m_vertexIndicesBufferSharedData;
		m_primIndicesBufferSharedData   = other.m_primIndicesBufferSharedData;
		m_meshBounds                    = std::move(other.m_meshBounds);
		m_meshDetails                   = other.m_meshDetails;

		return *this;
	}
};
#endif
