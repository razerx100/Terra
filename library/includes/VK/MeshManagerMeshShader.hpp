#ifndef MESH_MANAGER_MESH_SHADER_HPP_
#define MESH_MANAGER_MESH_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <VkResources.hpp>
#include <StagingBufferManager.hpp>
#include <CommonBuffers.hpp>

#include <MeshBundle.hpp>

class MeshManagerMeshShader
{
public:
	// In GLSL, vec3 are actually vec4. So, whether it is a vec3 or vec4, it would
	// take as much space as a vec4. And it is necessary here because this data
	// will be passed in a Storage Buffer.
	// Also, not really using this struct to hold data. Only to get the correct size.
	struct GLSLVertex
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 normal;
		DirectX::XMFLOAT2 uv;
		float             padding[2];
	};

	// For some reason in GLSL, array element needs to be the multiple of a Vec4 or I am missing something.
	struct GLSLMeshletDetails
	{
		MeshletDetails details;
		std::uint32_t  padding[2];
	};

	// The offset should be enough to identify the mesh. We wouldn't really need the size
	// as the meshlets should already have that data.
	struct MeshDetailsMS
	{
		std::uint32_t vertexOffset;
		std::uint32_t vertexIndicesOffset;
		std::uint32_t primIndicesOffset;
		std::uint32_t meshletOffset;
	};

	struct PerMeshBundleData
	{
		std::uint32_t meshOffset;
	};

public:
	MeshManagerMeshShader();

	void SetMeshBundle(
		std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
	);
	void SetMeshBundle(
		std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
		SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
		TemporaryDataBufferGPU& tempBuffer
	);

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
	const SharedBufferData& GetPerMeshletSharedData() const noexcept
	{
		return m_perMeshletBufferSharedData;
	}
	[[nodiscard]]
	const SharedBufferData& GetPerMeshSharedData() const noexcept { return m_perMeshSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetPerMeshBundleSharedData() const noexcept
	{
		return m_perMeshBundleSharedData;
	}

	[[nodiscard]]
	MeshDetailsMS GetMeshDetailsMS() const noexcept { return m_meshDetails; }
	[[nodiscard]]
	const MeshDetails& GetMeshDetails(size_t index) const noexcept
	{
		return m_bundleDetails.meshDetails[index];
	}

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(MeshDetails));
	}

private:
	static void ConfigureVertices(
		const std::vector<Vertex>& vertices, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& verticesSharedBuffer, SharedBufferData& verticesSharedData,
		std::uint32_t& verticesDetailOffset, TemporaryDataBufferGPU& tempBuffer
	) noexcept;

private:
	SharedBufferData  m_vertexBufferSharedData;
	SharedBufferData  m_vertexIndicesBufferSharedData;
	SharedBufferData  m_primIndicesBufferSharedData;
	SharedBufferData  m_perMeshletBufferSharedData;
	SharedBufferData  m_perMeshSharedData;
	SharedBufferData  m_perMeshBundleSharedData;
	MeshDetailsMS     m_meshDetails;
	MeshBundleDetails m_bundleDetails;

public:
	MeshManagerMeshShader(const MeshManagerMeshShader&) = delete;
	MeshManagerMeshShader& operator=(const MeshManagerMeshShader&) = delete;

	MeshManagerMeshShader(MeshManagerMeshShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_vertexIndicesBufferSharedData{ other.m_vertexIndicesBufferSharedData },
		m_primIndicesBufferSharedData{ other.m_primIndicesBufferSharedData },
		m_perMeshletBufferSharedData{ other.m_perMeshletBufferSharedData },
		m_perMeshSharedData{ other.m_perMeshSharedData },
		m_perMeshBundleSharedData{ other.m_perMeshBundleSharedData },
		m_meshDetails{ other.m_meshDetails },
		m_bundleDetails{ std::move(other.m_bundleDetails) }
	{}

	MeshManagerMeshShader& operator=(MeshManagerMeshShader&& other) noexcept
	{
		m_vertexBufferSharedData        = other.m_vertexBufferSharedData;
		m_vertexIndicesBufferSharedData = other.m_vertexIndicesBufferSharedData;
		m_primIndicesBufferSharedData   = other.m_primIndicesBufferSharedData;
		m_perMeshletBufferSharedData    = other.m_perMeshletBufferSharedData;
		m_perMeshSharedData             = other.m_perMeshSharedData;
		m_perMeshBundleSharedData       = other.m_perMeshBundleSharedData;
		m_meshDetails                   = other.m_meshDetails;
		m_bundleDetails                 = std::move(other.m_bundleDetails);

		return *this;
	}
};
#endif
