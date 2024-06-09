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
	struct TempData
	{
		std::unique_ptr<MeshBundleVS> meshBundle;
	};

public:
	MeshManagerVertexShader();

	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
		std::deque<TempData>& tempDataContainer
	);

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

	[[nodiscard]]
	const std::vector<MeshBound>& GetBounds() const noexcept { return m_meshBounds; }

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }
	[[nodiscard]]
	const SharedBufferData& GetIndexSharedData() const noexcept { return m_indexBufferSharedData; }

private:
	SharedBufferData       m_vertexBufferSharedData;
	SharedBufferData       m_indexBufferSharedData;
	std::vector<MeshBound> m_meshBounds;

public:
	MeshManagerVertexShader(const MeshManagerVertexShader&) = delete;
	MeshManagerVertexShader& operator=(const MeshManagerVertexShader&) = delete;

	MeshManagerVertexShader(MeshManagerVertexShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_indexBufferSharedData{ other.m_indexBufferSharedData },
		m_meshBounds{ std::move(other.m_meshBounds) }
	{}
	MeshManagerVertexShader& operator=(MeshManagerVertexShader&& other) noexcept
	{
		m_vertexBufferSharedData = other.m_vertexBufferSharedData;
		m_indexBufferSharedData  = other.m_indexBufferSharedData;
		m_meshBounds             = std::move(other.m_meshBounds);

		return *this;
	}
};
#endif
