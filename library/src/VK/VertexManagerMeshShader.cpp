#include <VertexManagerMeshShader.hpp>

VertexManagerMeshShader::VertexManagerMeshShader(VkDevice device, MemoryManager* memoryManager)
	: m_vertexBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_vertexIndicesBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_primIndicesBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_vertices{}, m_vertexIndices{}, m_primIndices{}
{}

void VertexManagerMeshShader::SetVerticesAndPrimIndices(
	std::vector<Vertex>&& vertices,
	std::vector<std::uint32_t>&& vertexIndices, std::vector<std::uint32_t>&& primIndices,
	StagingBufferManager& stagingBufferMan
) {
	std::vector<GLSLVertex> glslVertices = TransformVertices(vertices);

	ConfigureBuffer(std::move(glslVertices), m_vertices, m_vertexBuffer, stagingBufferMan);
	ConfigureBuffer(std::move(vertexIndices), m_vertexIndices, m_vertexIndicesBuffer, stagingBufferMan);
	ConfigureBuffer(std::move(primIndices), m_primIndices, m_primIndicesBuffer, stagingBufferMan);
}

void VertexManagerMeshShader::CleanupTempData() noexcept
{
	m_vertices      = std::vector<GLSLVertex>{};
	m_vertexIndices = std::vector<std::uint32_t>{};
	m_primIndices   = std::vector<std::uint32_t>{};
}

void VertexManagerMeshShader::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t verticesBindingSlot,
	std::uint32_t vertexIndicesBindingSlot, std::uint32_t primIndicesBindingSlot
) const noexcept {
	descriptorBuffer.AddStorageBufferDescriptor(m_vertexBuffer, verticesBindingSlot);
	descriptorBuffer.AddStorageBufferDescriptor(m_vertexIndicesBuffer, vertexIndicesBindingSlot);
	descriptorBuffer.AddStorageBufferDescriptor(m_primIndicesBuffer, primIndicesBindingSlot);
}

std::vector<VertexManagerMeshShader::GLSLVertex> VertexManagerMeshShader::TransformVertices(
	const std::vector<Vertex>& vertices
) noexcept {
	std::vector<GLSLVertex> glslVertices{ std::size(vertices) };

	for (size_t index = 0u; index < std::size(vertices); ++index) {
		glslVertices[index].position = vertices[index].position;
		glslVertices[index].normal   = vertices[index].normal;
		glslVertices[index].uv       = vertices[index].uv;
	}

	return glslVertices;
}
