#ifndef VK_MESH_BUNDLE_MS_HPP_
#define VK_MESH_BUNDLE_MS_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <VkResources.hpp>
#include <VkStagingBufferManager.hpp>
#include <VkSharedBuffers.hpp>

#include <MeshBundle.hpp>

namespace Terra
{
class VkMeshBundleMS
{
	using MeshBundleDetails_t = std::vector<MeshTemporaryDetailsMS>;

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

	// The offset should be enough to identify the mesh bundle. We wouldn't really need the size
	// as the meshlets should already have that data.
	struct MeshBundleDetailsMS
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
	VkMeshBundleMS();

	void SetMeshBundle(
		MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void SetMeshBundle(
		MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
		SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
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
	MeshBundleDetailsMS GetMeshBundleDetailsMS() const noexcept { return m_meshBundleDetails; }
	[[nodiscard]]
	const MeshTemporaryDetailsMS& GetMeshDetails(size_t index) const noexcept
	{
		return m_bundleDetails[index];
	}

	[[nodiscard]]
	static consteval std::uint32_t GetConstantBufferSize() noexcept
	{
		return static_cast<std::uint32_t>(sizeof(MeshBundleDetailsMS));
	}

private:
	void _setMeshBundle(
		MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
		SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	);

	static void ConfigureVertices(
		const std::vector<Vertex>& vertices, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& verticesSharedBuffer, SharedBufferData& verticesSharedData,
		std::uint32_t& verticesDetailOffset, Callisto::TemporaryDataBufferGPU& tempBuffer
	) noexcept;

private:
	SharedBufferData    m_vertexBufferSharedData;
	SharedBufferData    m_vertexIndicesBufferSharedData;
	SharedBufferData    m_primIndicesBufferSharedData;
	SharedBufferData    m_perMeshletBufferSharedData;
	SharedBufferData    m_perMeshSharedData;
	SharedBufferData    m_perMeshBundleSharedData;
	MeshBundleDetailsMS m_meshBundleDetails;
	MeshBundleDetails_t m_bundleDetails;

public:
	VkMeshBundleMS(const VkMeshBundleMS&) = delete;
	VkMeshBundleMS& operator=(const VkMeshBundleMS&) = delete;

	VkMeshBundleMS(VkMeshBundleMS&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_vertexIndicesBufferSharedData{ other.m_vertexIndicesBufferSharedData },
		m_primIndicesBufferSharedData{ other.m_primIndicesBufferSharedData },
		m_perMeshletBufferSharedData{ other.m_perMeshletBufferSharedData },
		m_perMeshSharedData{ other.m_perMeshSharedData },
		m_perMeshBundleSharedData{ other.m_perMeshBundleSharedData },
		m_meshBundleDetails{ other.m_meshBundleDetails },
		m_bundleDetails{ std::move(other.m_bundleDetails) }
	{}

	VkMeshBundleMS& operator=(VkMeshBundleMS&& other) noexcept
	{
		m_vertexBufferSharedData        = other.m_vertexBufferSharedData;
		m_vertexIndicesBufferSharedData = other.m_vertexIndicesBufferSharedData;
		m_primIndicesBufferSharedData   = other.m_primIndicesBufferSharedData;
		m_perMeshletBufferSharedData    = other.m_perMeshletBufferSharedData;
		m_perMeshSharedData             = other.m_perMeshSharedData;
		m_perMeshBundleSharedData       = other.m_perMeshBundleSharedData;
		m_meshBundleDetails             = other.m_meshBundleDetails;
		m_bundleDetails                 = std::move(other.m_bundleDetails);

		return *this;
	}
};
}
#endif
