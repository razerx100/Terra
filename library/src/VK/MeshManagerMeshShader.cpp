#include <MeshManagerMeshShader.hpp>

MeshManagerMeshShader::MeshManagerMeshShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u }, m_primIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_meshBounds{}, m_meshDetails{ 0u, 0u, 0u }
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

	auto ConfigureBuffer = []<typename T>
		(
			const std::vector<T>& elements, StagingBufferManager& stagingBufferMan,
			SharedBuffer& sharedBuffer, SharedBufferData& sharedData, std::uint32_t& detailOffset
		)
	{
		constexpr auto stride = static_cast<VkDeviceSize>(sizeof(T));
		const auto bufferSize = static_cast<VkDeviceSize>(stride * std::size(elements));

		sharedData   = sharedBuffer.AllocateAndGetSharedData(bufferSize);
		detailOffset = static_cast<std::uint32_t>(sharedData.offset / stride);

		stagingBufferMan.AddBuffer(
			std::data(elements), bufferSize, sharedData.bufferData, sharedData.offset,
			QueueType::GraphicsQueue, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT
		);
	};

	const std::vector<std::uint32_t>& vertexIndices = tempData.meshBundle->GetVertexIndices();
	const std::vector<std::uint32_t>& primIndices   = tempData.meshBundle->GetPrimIndices();

	ConfigureBuffer(
		tempData.vertices, stagingBufferMan, vertexSharedBuffer, m_vertexBufferSharedData,
		m_meshDetails.vertexOffset
	);
	ConfigureBuffer(
		vertexIndices, stagingBufferMan, vertexIndicesSharedBuffer, m_vertexIndicesBufferSharedData,
		m_meshDetails.vertexIndicesOffset
	);
	ConfigureBuffer(
		primIndices, stagingBufferMan, primIndicesSharedBuffer, m_primIndicesBufferSharedData,
		m_meshDetails.primIndicesOffset
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
