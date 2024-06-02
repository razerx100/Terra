#include <MeshManagerMeshShader.hpp>

MeshManagerMeshShader::MeshManagerMeshShader(VkDevice device, MemoryManager* memoryManager)
	: m_vertexBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_vertexIndicesBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_primIndicesBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_meshBounds{}
{}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	std::deque<TempData>& tempDataContainer
) {
	TempData& tempData = tempDataContainer.emplace_back(
		TempData{
			.meshBundle = std::move(meshBundle)
		}
	);

	{
		const std::vector<Vertex>& vertices = tempData.meshBundle->GetVertices();

		tempData.vertices = TransformVertices(vertices);

		// No need to keep two copies of the same data.
		tempData.meshBundle->CleanUpVertices();
	}

	ConfigureBuffer(tempData.vertices, m_vertexBuffer, stagingBufferMan);
	ConfigureBuffer(tempData.meshBundle->GetVertexIndices(), m_vertexIndicesBuffer, stagingBufferMan);
	ConfigureBuffer(tempData.meshBundle->GetPrimIndices(), m_primIndicesBuffer, stagingBufferMan);
}

void MeshManagerMeshShader::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t verticesBindingSlot,
	std::uint32_t vertexIndicesBindingSlot, std::uint32_t primIndicesBindingSlot
) const noexcept {
	descriptorBuffer.SetStorageBufferDescriptor(m_vertexBuffer, verticesBindingSlot, 0u);
	descriptorBuffer.SetStorageBufferDescriptor(m_vertexIndicesBuffer, vertexIndicesBindingSlot, 0u);
	descriptorBuffer.SetStorageBufferDescriptor(m_primIndicesBuffer, primIndicesBindingSlot, 0u);
}

std::vector<MeshManagerMeshShader::GLSLVertex> MeshManagerMeshShader::TransformVertices(
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
