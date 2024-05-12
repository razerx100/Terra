#ifndef MESH_MANAGER_VERTEX_SHADER_HPP_
#define MESH_MANAGER_VERTEX_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <VkCommandQueue.hpp>

#include <MeshBundle.hpp>

class MeshManagerVertexShader
{
public:
	MeshManagerVertexShader(VkDevice device, MemoryManager* memoryManager) noexcept;

	void SetMeshBundle(
		std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan
	);

	void Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;
	void CleanupTempData() noexcept;

	[[nodiscard]]
	const std::vector<MeshBounds>& GetBounds() const noexcept { return m_meshBundle->GetBounds(); }

private:
	Buffer                        m_vertexBuffer;
	Buffer                        m_indexBuffer;
	std::unique_ptr<MeshBundleVS> m_meshBundle;

public:
	MeshManagerVertexShader(const MeshManagerVertexShader&) = delete;
	MeshManagerVertexShader& operator=(const MeshManagerVertexShader&) = delete;

	MeshManagerVertexShader(MeshManagerVertexShader&& other) noexcept
		: m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) },
		m_meshBundle{ std::move(other.m_meshBundle) }
	{}
	MeshManagerVertexShader& operator=(MeshManagerVertexShader&& other) noexcept
	{
		m_vertexBuffer = std::move(other.m_vertexBuffer);
		m_indexBuffer  = std::move(other.m_indexBuffer);
		m_meshBundle   = std::move(other.m_meshBundle);

		return *this;
	}
};
#endif
