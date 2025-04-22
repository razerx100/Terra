#include <VkMeshBundleMS.hpp>
#include <VectorToSharedPtr.hpp>

namespace Terra
{
VkMeshBundleMS::VkMeshBundleMS()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u },
	m_vertexIndicesBufferSharedData{ nullptr, 0u, 0u }, m_primIndicesBufferSharedData{ nullptr, 0u, 0u },
	m_perMeshletBufferSharedData{ nullptr, 0u, 0u }, m_perMeshSharedData{ nullptr, 0u, 0u },
	m_perMeshBundleSharedData{ nullptr, 0u, 0u }, m_meshBundleDetails{ 0u, 0u, 0u, 0u },
	m_bundleDetails{}
{}

void VkMeshBundleMS::_setMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	const std::vector<Vertex>& vertices  = meshBundle->GetVertices();

	auto ConfigureBuffer = []<typename T>
		(
			const std::vector<T>& elements, StagingBufferManager& stagingBufferMan,
			SharedBufferGPU& sharedBuffer, SharedBufferData& sharedData,
			std::uint32_t& detailOffset, Callisto::TemporaryDataBufferGPU& tempBuffer
		)
	{
		constexpr auto stride = static_cast<VkDeviceSize>(sizeof(T));
		const auto bufferSize = static_cast<VkDeviceSize>(stride * std::size(elements));

		sharedData   = sharedBuffer.AllocateAndGetSharedData(bufferSize, tempBuffer);
		detailOffset = static_cast<std::uint32_t>(sharedData.offset / stride);

		std::shared_ptr<std::uint8_t[]> tempDataBuffer = Callisto::CopyVectorToSharedPtr(elements);

		stagingBufferMan.AddBuffer(
			std::move(tempDataBuffer), bufferSize, sharedData.bufferData, sharedData.offset,
			tempBuffer
		);
	};

	const std::vector<std::uint32_t>& vertexIndices   = meshBundle->GetVertexIndices();
	const std::vector<std::uint32_t>& primIndices     = meshBundle->GetPrimIndices();
	const std::vector<MeshletDetails>& meshletDetails = meshBundle->GetMeshletDetails();

	ConfigureVertices(
		vertices, stagingBufferMan, vertexSharedBuffer, m_vertexBufferSharedData,
		m_meshBundleDetails.vertexOffset, tempBuffer
	);
	ConfigureBuffer(
		vertexIndices, stagingBufferMan, vertexIndicesSharedBuffer, m_vertexIndicesBufferSharedData,
		m_meshBundleDetails.vertexIndicesOffset, tempBuffer
	);
	ConfigureBuffer(
		primIndices, stagingBufferMan, primIndicesSharedBuffer, m_primIndicesBufferSharedData,
		m_meshBundleDetails.primIndicesOffset, tempBuffer
	);
	ConfigureBuffer(
		meshletDetails, stagingBufferMan, perMeshletSharedBuffer, m_perMeshletBufferSharedData,
		m_meshBundleDetails.meshletOffset, tempBuffer
	);

	m_bundleDetails = std::move(meshBundle->GetTemporaryBundleDetails());
}

void VkMeshBundleMS::SetMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	// Init the temp data.
	meshBundle->GenerateTemporaryData(true);

	_setMeshBundle(
		std::move(meshBundle),
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		perMeshletSharedBuffer, tempBuffer
	);
}

void VkMeshBundleMS::SetMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& vertexIndicesSharedBuffer,
	SharedBufferGPU& primIndicesSharedBuffer, SharedBufferGPU& perMeshletSharedBuffer,
	SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	constexpr auto perMeshStride = sizeof(AxisAlignedBoundingBox);

	// Init the temp data.
	meshBundle->GenerateTemporaryData(true);

	// Need this or else the overload which returns the R value ref will be called.
	const MeshBundleTemporary& meshBundleR = *meshBundle;
	const std::vector<MeshTemporaryDetailsMS>& meshDetailsMS
		= meshBundleR.GetTemporaryBundleDetails().meshTemporaryDetailsMS;

	const size_t meshCount       = std::size(meshDetailsMS);
	const auto perMeshBufferSize = static_cast<VkDeviceSize>(perMeshStride * meshCount);

	m_perMeshSharedData    = perMeshSharedBuffer.AllocateAndGetSharedData(perMeshBufferSize, tempBuffer);

	auto perMeshBufferData = std::make_shared<std::uint8_t[]>(perMeshBufferSize);

	{
		size_t perMeshOffset             = 0u;
		std::uint8_t* perMeshBufferStart = perMeshBufferData.get();

		for (const MeshTemporaryDetailsMS& meshDetail : meshDetailsMS)
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

	_setMeshBundle(
		std::move(meshBundle),
		stagingBufferMan, vertexSharedBuffer, vertexIndicesSharedBuffer, primIndicesSharedBuffer,
		perMeshletSharedBuffer, tempBuffer
	);
}

void VkMeshBundleMS::ConfigureVertices(
	const std::vector<Vertex>& vertices, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& verticesSharedBuffer, SharedBufferData& verticesSharedData,
	std::uint32_t& verticesDetailOffset, Callisto::TemporaryDataBufferGPU& tempBuffer
) noexcept {
	constexpr auto vertexStride = static_cast<VkDeviceSize>(sizeof(GLSLVertex));
	const size_t vertexCount    = std::size(vertices);
	const auto vertexBufferSize = static_cast<VkDeviceSize>(vertexStride * vertexCount);

	verticesSharedData   = verticesSharedBuffer.AllocateAndGetSharedData(vertexBufferSize, tempBuffer);
	verticesDetailOffset = static_cast<std::uint32_t>(verticesSharedData.offset / vertexStride);

	auto verticesTempDataBuffer = std::make_shared<std::uint8_t[]>(vertexBufferSize);

	{
		std::uint8_t* bufferStart = verticesTempDataBuffer.get();
		size_t bufferOffset       = 0u;

		for (size_t index = 0u; index < vertexCount; ++index)
		{
			// In GLSL storage buffer, when multiple variables are laid out in the same structure,
			// the vec3 would actually be vec4. So, the next member would start after 16bytes.
			// This is not needed in the Vertex shader since we pass the vertices through the
			// Input Assembler but necessary in the Mesh Shader, as we pass the vertices as
			// Storage Buffer.
			constexpr size_t vec4GLSLOffsetSize = sizeof(DirectX::XMFLOAT4);

			const Vertex& vertex = vertices[index];

			// Position
			memcpy(
				bufferStart + bufferOffset, &vertex.position, sizeof(DirectX::XMFLOAT3)
			);

			// Normal
			memcpy(
				bufferStart + bufferOffset + vec4GLSLOffsetSize,
				&vertex.normal, sizeof(DirectX::XMFLOAT3)
			);

			// UV
			memcpy(
				bufferStart + bufferOffset + vec4GLSLOffsetSize + vec4GLSLOffsetSize,
				&vertex.uv, sizeof(DirectX::XMFLOAT2)
			);

			bufferOffset += vertexStride;
		}
	}

	stagingBufferMan.AddBuffer(
		std::move(verticesTempDataBuffer), vertexBufferSize, verticesSharedData.bufferData,
		verticesSharedData.offset, tempBuffer
	);
}
}
