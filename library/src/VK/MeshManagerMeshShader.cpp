#include <MeshManagerMeshShader.hpp>

MeshManagerMeshShader::MeshManagerMeshShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u }, m_primIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_meshBoundsSharedData{ nullptr, 0u, 0u }, m_meshDetails{ 0u, 0u, 0u }
{}

void MeshManagerMeshShader::SetMeshBundle(
	StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& vertexIndicesSharedBuffer,
	SharedBuffer& primIndicesSharedBuffer, TemporaryDataBuffer& tempBuffer, TempData& tempData
) {
	{
		const std::vector<Vertex>& vertices = tempData.meshBundle->GetVertices();

		tempData.vertices = TransformVertices(vertices);

		// No need to keep two copies of the same data.
		tempData.meshBundle->CleanUpVertices();
	}

	auto ConfigureBuffer = []<typename T>
		(
			const std::vector<T>& elements, StagingBufferManager& stagingBufferMan,
			SharedBuffer& sharedBuffer, SharedBufferData& sharedData,
			std::uint32_t& detailOffset, TemporaryDataBuffer& tempBuffer
		)
	{
		constexpr auto stride = static_cast<VkDeviceSize>(sizeof(T));
		const auto bufferSize = static_cast<VkDeviceSize>(stride * std::size(elements));

		sharedData   = sharedBuffer.AllocateAndGetSharedData(bufferSize, tempBuffer);
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
		m_meshDetails.vertexOffset, tempBuffer
	);
	ConfigureBuffer(
		vertexIndices, stagingBufferMan, vertexIndicesSharedBuffer, m_vertexIndicesBufferSharedData,
		m_meshDetails.vertexIndicesOffset, tempBuffer
	);
	ConfigureBuffer(
		primIndices, stagingBufferMan, primIndicesSharedBuffer, m_primIndicesBufferSharedData,
		m_meshDetails.primIndicesOffset, tempBuffer
	);
}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& vertexIndicesSharedBuffer,
	SharedBuffer& primIndicesSharedBuffer,
	TemporaryDataBuffer& tempBuffer, std::deque<TempData>& tempDataContainer
) {
	TempData& tempData = tempDataContainer.emplace_back(
		TempData{
			.meshBundle = std::move(meshBundle)
		}
	);

	SetMeshBundle(
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		tempBuffer, tempData
	);
}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& vertexIndicesSharedBuffer,
	SharedBuffer& primIndicesSharedBuffer, SharedBuffer& boundsSharedBuffer,
	TemporaryDataBuffer& tempBuffer, std::deque<TempDataBounds>& tempDataContainer,
	QueueType dstQueue, VkPipelineStageFlagBits2 dstPipelineStage
) {
	TempDataBounds& tempDataBounds = tempDataContainer.emplace_back(
		TempDataBounds{
			.tempData = TempData{ .meshBundle = std::move(meshBundle) }
		}
	);
	TempData& tempData = tempDataBounds.tempData;

	SetMeshBundle(
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		tempBuffer, tempData
	);

	const std::vector<MeshBound>& bounds = tempData.meshBundle->GetBounds();

	constexpr auto boundStride = sizeof(MeshBound);
	const auto boundSize       = static_cast<VkDeviceSize>(boundStride * std::size(bounds));

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	stagingBufferMan.AddBuffer(
		std::data(bounds), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset,
		dstQueue, VK_ACCESS_2_SHADER_READ_BIT, dstPipelineStage
	);
}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& vertexIndicesSharedBuffer,
	SharedBuffer& primIndicesSharedBuffer, SharedBuffer& boundsSharedBuffer,
	TemporaryDataBuffer& tempBuffer, std::deque<TempDataBounds>& tempDataContainer
) {
	TempDataBounds& tempDataBounds = tempDataContainer.emplace_back(
		TempDataBounds{
			.tempData = TempData{ .meshBundle = std::move(meshBundle) }
		}
	);
	TempData& tempData = tempDataBounds.tempData;

	SetMeshBundle(
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		tempBuffer, tempData
	);

	const std::vector<MeshBound>& bounds = tempData.meshBundle->GetBounds();

	constexpr auto boundStride = sizeof(MeshBound);
	const auto boundSize       = static_cast<VkDeviceSize>(boundStride * std::size(bounds));

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	stagingBufferMan.AddBuffer(
		std::data(bounds), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset
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

MeshManagerMeshShader::BoundsDetails MeshManagerMeshShader::GetBoundsDetails() const noexcept
{
	constexpr auto stride = sizeof(MeshBound);

	return BoundsDetails{
		.offset = static_cast<std::uint32_t>(m_meshBoundsSharedData.offset / stride),
		.count  = static_cast<std::uint32_t>(m_meshBoundsSharedData.size / stride)
	};
}
