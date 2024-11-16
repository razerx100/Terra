#include <MeshManagerMeshShader.hpp>
#include <VectorToSharedPtr.hpp>

MeshManagerMeshShader::MeshManagerMeshShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u }, m_primIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_perMeshletBufferSharedData{ nullptr, 0u, 0u }, m_perMeshSharedData{ nullptr, 0u, 0u },
	m_perMeshBundleSharedData{ nullptr, 0u, 0u }, m_meshDetails{ 0u, 0u, 0u }, m_bundleDetails{}
{}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer
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
			tempBuffer
		);
	};

	const std::vector<std::uint32_t>& vertexIndices   = meshBundle->GetVertexIndices();
	const std::vector<std::uint32_t>& primIndices     = meshBundle->GetPrimIndices();
	const std::vector<MeshletDetails>& meshletDetails = meshBundle->GetMeshletDetails();

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
	ConfigureBuffer(
		meshletDetails, stagingBufferMan, perMeshletSharedBuffer, m_perMeshletBufferSharedData,
		m_meshDetails.meshletOffset, tempBuffer
	);

	m_bundleDetails = std::move(meshBundle->GetBundleDetails());
}

void MeshManagerMeshShader::SetMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
	SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer
) {
	constexpr auto perMeshStride = sizeof(AxisAlignedBoundingBox);

	// Need this or else the overload which returns the R value ref will be called.
	const MeshBundleMS& meshBundleR             = *meshBundle;
	const std::vector<MeshDetails>& meshDetails = meshBundleR.GetBundleDetails().meshDetails;

	const size_t meshCount       = std::size(meshDetails);
	const auto perMeshBufferSize = static_cast<VkDeviceSize>(perMeshStride * meshCount);

	m_perMeshSharedData    = perMeshSharedBuffer.AllocateAndGetSharedData(perMeshBufferSize, tempBuffer);

	auto perMeshBufferData = std::make_shared<std::uint8_t[]>(perMeshBufferSize);

	{
		size_t perMeshOffset             = 0u;
		std::uint8_t* perMeshBufferStart = perMeshBufferData.get();

		for (const MeshDetails& meshDetail : meshDetails)
		{
			memcpy(perMeshBufferStart + perMeshOffset, &meshDetail.aabb, perMeshStride);

			perMeshOffset += perMeshStride;
		}
	}

	// Mesh Bundle Data
	constexpr size_t perMeshBundleDataSize = sizeof(PerMeshBundleData);

	auto perBundleData        = std::make_shared<std::uint8_t[]>(perMeshBundleDataSize);

	m_perMeshBundleSharedData = perMeshBundleSharedBuffer.AllocateAndGetSharedData(
		perMeshBundleDataSize, tempBuffer
	);

	{
		PerMeshBundleData bundleData
		{
			.meshOffset = static_cast<std::uint32_t>(m_perMeshSharedData.offset / perMeshStride)
		};

		memcpy(perBundleData.get(), &bundleData, perMeshBundleDataSize);
	}

	stagingBufferMan.AddBuffer(
		std::move(perBundleData), perMeshBundleDataSize,
		m_perMeshBundleSharedData.bufferData, m_perMeshBundleSharedData.offset, tempBuffer
	);

	stagingBufferMan.AddBuffer(
		std::move(perMeshBufferData), perMeshBufferSize,
		m_perMeshSharedData.bufferData, m_perMeshSharedData.offset, tempBuffer
	);

	SetMeshBundle(
		std::move(meshBundle),
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		perMeshletSharedBuffer, tempBuffer
	);
}

std::vector<MeshManagerMeshShader::GLSLVertex> MeshManagerMeshShader::TransformVertices(
	const std::vector<Vertex>& vertices
) noexcept {
	std::vector<GLSLVertex> glslVertices{ std::size(vertices) };

	for (size_t index = 0u; index < std::size(vertices); ++index)
	{
		glslVertices[index].position = vertices[index].position;
		glslVertices[index].normal   = vertices[index].normal;
		glslVertices[index].uv       = vertices[index].uv;
	}

	return glslVertices;
}
