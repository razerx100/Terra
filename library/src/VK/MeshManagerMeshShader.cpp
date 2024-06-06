#include <MeshManagerMeshShader.hpp>

MeshManagerMeshShader::MeshManagerMeshShader(VkDevice device, MemoryManager* memoryManager)
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_primIndicesBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_meshBounds{}
{}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& vertexIndicesBuffer,
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

	const std::vector<std::uint32_t>& vertexIndices = tempData.meshBundle->GetVertexIndices();

	const auto vertexBufferSize
		= static_cast<VkDeviceSize>(sizeof(Vertex) * std::size(tempData.vertices));
	const auto vertexIndicesBufferSize
		= static_cast<VkDeviceSize>(sizeof(std::uint32_t) * std::size(vertexIndices));

	m_vertexBufferSharedData        = vertexSharedBuffer.AllocateAndGetSharedData(vertexBufferSize);
	m_vertexIndicesBufferSharedData = vertexIndicesBuffer.AllocateAndGetSharedData(vertexIndicesBufferSize);

	stagingBufferMan.AddBuffer(
		std::data(tempData.vertices), vertexBufferSize, m_vertexBufferSharedData.bufferData,
		m_vertexBufferSharedData.offset
	);
	stagingBufferMan.AddBuffer(
		std::data(vertexIndices), vertexIndicesBufferSize, m_vertexIndicesBufferSharedData.bufferData,
		m_vertexIndicesBufferSharedData.offset
	);

	//ConfigureBuffer(tempData.meshBundle->GetPrimIndices(), m_primIndicesBuffer, stagingBufferMan);
}

void MeshManagerMeshShader::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t verticesBindingSlot,
	std::uint32_t vertexIndicesBindingSlot, std::uint32_t primIndicesBindingSlot
) const noexcept {
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
