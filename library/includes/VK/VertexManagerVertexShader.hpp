#ifndef VERTEX_MANAGER_VERTEX_SHADER_HPP_
#define VERTEX_MANAGER_VERTEX_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <VkCommandQueue.hpp>

#include <Model.hpp>

class VertexManagerVertexShader
{
public:
	VertexManagerVertexShader(VkDevice device, MemoryManager* memoryManager) noexcept;

	void SetVerticesAndIndices(
		std::vector<Vertex>&& vertices, std::vector<std::uint32_t>&& indices,
		StagingBufferManager& stagingBufferMan
	) noexcept;

	void Bind(VKCommandBuffer& graphicsCmdBuffer) const noexcept;
	void CleanupTempData() noexcept;

private:
	Buffer                     m_vertexBuffer;
	Buffer                     m_indexBuffer;
	std::vector<Vertex>        m_vertices;
	std::vector<std::uint32_t> m_indices;

public:
	VertexManagerVertexShader(const VertexManagerVertexShader&) = delete;
	VertexManagerVertexShader& operator=(const VertexManagerVertexShader&) = delete;

	VertexManagerVertexShader(VertexManagerVertexShader&& other) noexcept
		: m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) },
		m_vertices{ std::move(other.m_vertices) }, m_indices{ std::move(other.m_indices) }
	{}
	VertexManagerVertexShader& operator=(VertexManagerVertexShader&& other) noexcept
	{
		m_vertexBuffer = std::move(other.m_vertexBuffer);
		m_indexBuffer  = std::move(other.m_indexBuffer);
		m_vertices     = std::move(other.m_vertices);
		m_indices      = std::move(other.m_indices);

		return *this;
	}
};
#endif
