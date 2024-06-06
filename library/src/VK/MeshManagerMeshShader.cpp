#include <MeshManagerMeshShader.hpp>

MeshManagerMeshShader::MeshManagerMeshShader(VkDevice device, MemoryManager* memoryManager)
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u }, m_primIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_meshBounds{}
{}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& vertexIndicesSharedBuffer,
	SharedBuffer& primIndicesSharedBuffer, std::deque<TempData>& tempDataContainer
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
	const std::vector<std::uint32_t>& primIndices   = tempData.meshBundle->GetPrimIndices();

	const auto vertexBufferSize
		= static_cast<VkDeviceSize>(sizeof(Vertex) * std::size(tempData.vertices));
	const auto vertexIndicesBufferSize
		= static_cast<VkDeviceSize>(sizeof(std::uint32_t) * std::size(vertexIndices));
	const auto primIndicesBufferSize
		= static_cast<VkDeviceSize>(sizeof(std::uint32_t) * std::size(primIndices));

	m_vertexBufferSharedData
		= vertexSharedBuffer.AllocateAndGetSharedData(vertexBufferSize);
	m_vertexIndicesBufferSharedData
		= vertexIndicesSharedBuffer.AllocateAndGetSharedData(vertexIndicesBufferSize);
	m_primIndicesBufferSharedData
		= primIndicesSharedBuffer.AllocateAndGetSharedData(primIndicesBufferSize);

	stagingBufferMan.AddBuffer(
		std::data(tempData.vertices), vertexBufferSize, m_vertexBufferSharedData.bufferData,
		m_vertexBufferSharedData.offset
	);
	stagingBufferMan.AddBuffer(
		std::data(vertexIndices), vertexIndicesBufferSize, m_vertexIndicesBufferSharedData.bufferData,
		m_vertexIndicesBufferSharedData.offset
	);
	stagingBufferMan.AddBuffer(
		std::data(primIndices), primIndicesBufferSize, m_primIndicesBufferSharedData.bufferData,
		m_primIndicesBufferSharedData.offset
	);
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
