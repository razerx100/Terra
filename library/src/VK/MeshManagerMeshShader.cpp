#include <MeshManagerMeshShader.hpp>
#include <VectorToSharedPtr.hpp>

MeshManagerMeshShader::MeshManagerMeshShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u }, m_primIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_meshBoundsSharedData{ nullptr, 0u, 0u }, m_meshDetails{ 0u, 0u, 0u }
{}

void MeshManagerMeshShader::SetMeshBundle(
	StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, TemporaryDataBufferGPU& tempBuffer,
	std::unique_ptr<MeshBundleMS> meshBundle
) {
	const std::vector<Vertex>& vertices  = meshBundle->GetVertices();

	std::vector<GLSLVertex> glslVertices = TransformVertices(vertices);

	auto ConfigureBuffer = []<typename T>
		(
			const std::vector<T>& elements, StagingBufferManager& stagingBufferMan,
			SharedBufferGPU& sharedBuffer, SharedBufferData& sharedData,
			std::uint32_t& detailOffset, TemporaryDataBufferGPU& tempBuffer
		)
	{
		constexpr auto stride = static_cast<VkDeviceSize>(sizeof(T));
		const auto bufferSize = static_cast<VkDeviceSize>(stride * std::size(elements));

		sharedData   = sharedBuffer.AllocateAndGetSharedData(bufferSize, tempBuffer);
		detailOffset = static_cast<std::uint32_t>(sharedData.offset / stride);

		std::shared_ptr<std::uint8_t[]> tempDataBuffer = CopyVectorToSharedPtr(elements);

		stagingBufferMan.AddBuffer(
			std::move(tempDataBuffer), bufferSize, sharedData.bufferData, sharedData.offset,
			QueueType::GraphicsQueue, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT, tempBuffer
		);
	};

	const std::vector<std::uint32_t>& vertexIndices = meshBundle->GetVertexIndices();
	const std::vector<std::uint32_t>& primIndices   = meshBundle->GetPrimIndices();

	ConfigureBuffer(
		glslVertices, stagingBufferMan, vertexSharedBuffer, m_vertexBufferSharedData,
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
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, TemporaryDataBufferGPU& tempBuffer
) {
	SetMeshBundle(
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		tempBuffer, std::move(meshBundle)
	);
}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& boundsSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer, QueueType dstQueue, VkPipelineStageFlagBits2 dstPipelineStage
) {
	const std::vector<MeshBound>& bounds = meshBundle->GetBounds();

	constexpr auto boundStride = sizeof(MeshBound);
	const auto boundSize       = static_cast<VkDeviceSize>(boundStride * std::size(bounds));

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	std::shared_ptr<std::uint8_t[]> boundBufferData = CopyVectorToSharedPtr(bounds);

	stagingBufferMan.AddBuffer(
		std::move(boundBufferData), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset,
		dstQueue, VK_ACCESS_2_SHADER_READ_BIT, dstPipelineStage, tempBuffer
	);

	SetMeshBundle(
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		tempBuffer, std::move(meshBundle)
	);
}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& boundsSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer
) {
	const std::vector<MeshBound>& bounds = meshBundle->GetBounds();

	constexpr auto boundStride = sizeof(MeshBound);
	const auto boundSize       = static_cast<VkDeviceSize>(boundStride * std::size(bounds));

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	std::shared_ptr<std::uint8_t[]> boundBufferData = CopyVectorToSharedPtr(bounds);

	stagingBufferMan.AddBuffer(
		std::move(boundBufferData), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset, tempBuffer
	);

	SetMeshBundle(
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		tempBuffer, std::move(meshBundle)
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
