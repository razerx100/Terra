#ifndef VK_MESH_BUNDLE_VS_HPP_
#define VK_MESH_BUNDLE_VS_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <VkResources.hpp>
#include <VkStagingBufferManager.hpp>
#include <VkCommandQueue.hpp>
#include <VkSharedBuffers.hpp>

#include <MeshBundle.hpp>

namespace Terra
{
class VkMeshBundleVS
{
	struct PerMeshBundleData
	{
		std::uint32_t meshOffset;
	};

public:
	VkMeshBundleVS();

	void SetMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void SetMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	);

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetIndexSharedData() const noexcept { return m_indexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetPerMeshSharedData() const noexcept { return m_perMeshSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetPerMeshBundleSharedData() const noexcept
	{
		return m_perMeshBundleSharedData;
	}

	[[nodiscard]]
	const MeshTemporaryDetailsVS& GetMeshDetails(size_t index) const noexcept
	{
		return m_bundleDetails.meshTemporaryDetailsVS[index];
	}

private:
	void _setMeshBundle(
		std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	);

private:
	SharedBufferData           m_vertexBufferSharedData;
	SharedBufferData           m_indexBufferSharedData;
	SharedBufferData           m_perMeshSharedData;
	SharedBufferData           m_perMeshBundleSharedData;
	MeshBundleTemporaryDetails m_bundleDetails;

public:
	VkMeshBundleVS(const VkMeshBundleVS&) = delete;
	VkMeshBundleVS& operator=(const VkMeshBundleVS&) = delete;

	VkMeshBundleVS(VkMeshBundleVS&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_indexBufferSharedData{ other.m_indexBufferSharedData },
		m_perMeshSharedData{ other.m_perMeshSharedData },
		m_perMeshBundleSharedData{ other.m_perMeshBundleSharedData },
		m_bundleDetails{ std::move(other.m_bundleDetails) }
	{}
	VkMeshBundleVS& operator=(VkMeshBundleVS&& other) noexcept
	{
		m_vertexBufferSharedData  = other.m_vertexBufferSharedData;
		m_indexBufferSharedData   = other.m_indexBufferSharedData;
		m_perMeshSharedData       = other.m_perMeshSharedData;
		m_perMeshBundleSharedData = other.m_perMeshBundleSharedData;
		m_bundleDetails           = std::move(other.m_bundleDetails);

		return *this;
	}
};
}
#endif
