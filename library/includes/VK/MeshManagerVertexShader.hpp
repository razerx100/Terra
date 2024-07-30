#ifndef MESH_MANAGER_VERTEX_SHADER_HPP_
#define MESH_MANAGER_VERTEX_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <deque>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <VkCommandQueue.hpp>
#include <CommonBuffers.hpp>

#include <MeshBundle.hpp>

class MeshManagerVertexShader
{
public:
	struct BoundsDetails
	{
		std::uint32_t offset;
		std::uint32_t count;
	};

public:
	MeshManagerVertexShader();

	// Without bound data.
	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
		TemporaryDataBuffer& tempBuffer
	);
	// With bound data when the bound data has exclusive ownership.
	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
		SharedBuffer& boundsSharedBuffer, TemporaryDataBuffer& tempBuffer,
		QueueType dstQueue, VkPipelineStageFlagBits2 dstPipelineStage
	);
	// With bound data when the bound data has shared ownership.
	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
		SharedBuffer& boundsSharedBuffer, TemporaryDataBuffer& tempBuffer
	);

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetIndexSharedData() const noexcept { return m_indexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetBoundsSharedData() const noexcept { return m_meshBoundsSharedData; }

	[[nodiscard]]
	BoundsDetails GetBoundsDetails() const noexcept;

private:
	void SetMeshBundle(
		StagingBufferManager& stagingBufferMan,
		SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
		TemporaryDataBuffer& tempBuffer, std::unique_ptr<MeshBundleVS> meshBundle
	);

private:
	SharedBufferData m_vertexBufferSharedData;
	SharedBufferData m_indexBufferSharedData;
	SharedBufferData m_meshBoundsSharedData;

public:
	MeshManagerVertexShader(const MeshManagerVertexShader&) = delete;
	MeshManagerVertexShader& operator=(const MeshManagerVertexShader&) = delete;

	MeshManagerVertexShader(MeshManagerVertexShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_indexBufferSharedData{ other.m_indexBufferSharedData },
		m_meshBoundsSharedData{ other.m_meshBoundsSharedData }
	{}
	MeshManagerVertexShader& operator=(MeshManagerVertexShader&& other) noexcept
	{
		m_vertexBufferSharedData = other.m_vertexBufferSharedData;
		m_indexBufferSharedData  = other.m_indexBufferSharedData;
		m_meshBoundsSharedData   = other.m_meshBoundsSharedData;

		return *this;
	}
};
#endif
