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
	MeshManagerVertexShader(VkDevice device, MemoryManager* memoryManager) noexcept;

	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
		SharedBuffer& vertexSharedBuffer,
		std::deque<TempData>& tempDataContainer
	);

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

	[[nodiscard]]
	const std::vector<MeshBounds>& GetBounds() const noexcept { return m_meshBounds; }

	[[nodiscard]]
	const SharedBufferData& GetVertexSharedData() const noexcept { return m_vertexBufferSharedData; }

private:
	SharedBufferData        m_vertexBufferSharedData;
	Buffer                  m_indexBuffer;
	std::vector<MeshBounds> m_meshBounds;

public:
	MeshManagerVertexShader(const MeshManagerVertexShader&) = delete;
	MeshManagerVertexShader& operator=(const MeshManagerVertexShader&) = delete;

	MeshManagerVertexShader(MeshManagerVertexShader&& other) noexcept
		: m_vertexBufferSharedData{ other.m_vertexBufferSharedData },
		m_indexBuffer{ std::move(other.m_indexBuffer) },
		m_meshBounds{ std::move(other.m_meshBounds) }
	{}
	MeshManagerVertexShader& operator=(MeshManagerVertexShader&& other) noexcept
	{
		m_vertexBufferSharedData = other.m_vertexBufferSharedData;
		m_indexBuffer            = std::move(other.m_indexBuffer);
		m_meshBounds             = std::move(other.m_meshBounds);

		return *this;
	}
};
#endif
