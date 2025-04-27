#include <VkMeshBundleVS.hpp>
#include <VectorToSharedPtr.hpp>

namespace Terra
{
VkMeshBundleVS::VkMeshBundleVS()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u }, m_indexBufferSharedData{ nullptr, 0u, 0u },
	m_perMeshSharedData{ nullptr, 0u, 0u }, m_perMeshBundleSharedData{ nullptr, 0u, 0u },
	m_bundleDetails {}
{}

void VkMeshBundleVS::_setMeshBundle(
	MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	// Vertex Buffer
	{
		const std::vector<Vertex>& vertices = meshBundle.vertices;

		const auto vertexBufferSize = static_cast<VkDeviceSize>(
			sizeof(Vertex) * std::size(vertices)
		);

		m_vertexBufferSharedData = vertexSharedBuffer.AllocateAndGetSharedData(
			vertexBufferSize, tempBuffer
		);

		std::shared_ptr<std::uint8_t[]> vertexBufferData = Callisto::CopyVectorToSharedPtr(
			vertices
		);

		stagingBufferMan.AddBuffer(
			std::move(vertexBufferData), vertexBufferSize,
			m_vertexBufferSharedData.bufferData, m_vertexBufferSharedData.offset,
			tempBuffer
		);
	}

	// Index Buffer
	{
		const std::vector<std::uint32_t>& indices = meshBundle.indices;

		const auto indexBufferSize = static_cast<VkDeviceSize>(
			sizeof(std::uint32_t) * std::size(indices)
		);

		m_indexBufferSharedData = indexSharedBuffer.AllocateAndGetSharedData(
			indexBufferSize, tempBuffer
		);

		std::shared_ptr<std::uint8_t[]> indexBufferData = Callisto::CopyVectorToSharedPtr(indices);

		stagingBufferMan.AddBuffer(
			std::move(indexBufferData), indexBufferSize,
			m_indexBufferSharedData.bufferData, m_indexBufferSharedData.offset,
			tempBuffer
		);
	}

	m_bundleDetails = std::move(meshBundle.bundleDetails.meshTemporaryDetailsVS);
}

void VkMeshBundleVS::SetMeshBundle(
	MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	_setMeshBundle(
		std::move(meshBundle), stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer
	);
}

void VkMeshBundleVS::SetMeshBundle(
	MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBufferGPU& vertexSharedBuffer, SharedBufferGPU& indexSharedBuffer,
	SharedBufferGPU& perMeshSharedBuffer, SharedBufferGPU& perMeshBundleSharedBuffer,
	Callisto::TemporaryDataBufferGPU& tempBuffer
) {
	constexpr size_t perMeshStride = sizeof(AxisAlignedBoundingBox);

	const std::vector<MeshTemporaryDetailsVS>& meshDetailsVS
		= meshBundle.bundleDetails.meshTemporaryDetailsVS;

	const size_t meshCount       = std::size(meshDetailsVS);
	const auto perMeshBufferSize = static_cast<VkDeviceSize>(perMeshStride * meshCount);

	m_perMeshSharedData = perMeshSharedBuffer.AllocateAndGetSharedData(
		perMeshBufferSize, tempBuffer
	);

	auto perMeshBufferData = std::make_shared<std::uint8_t[]>(perMeshBufferSize);

	{
		size_t perMeshOffset             = 0u;
		std::uint8_t* perMeshBufferStart = perMeshBufferData.get();

		for (const MeshTemporaryDetailsVS& meshDetail : meshDetailsVS)
		{
			memcpy(perMeshBufferStart + perMeshOffset, &meshDetail.aabb, perMeshStride);

			perMeshOffset += perMeshStride;
		}
	}

	// Mesh Bundle Data
	constexpr size_t perMeshBundleDataSize = sizeof(PerMeshBundleData);

	auto perBundleData  = std::make_shared<std::uint8_t[]>(perMeshBundleDataSize);

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
		std::move(meshBundle), stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer
	);
}

void VkMeshBundleVS::Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
{
	VkBuffer vertexBuffers[]           = { m_vertexBufferSharedData.bufferData->Get() };
	const VkDeviceSize vertexOffsets[] = { m_vertexBufferSharedData.offset };

	VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

	vkCmdBindVertexBuffers(cmdBuffer, 0u, 1u, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(
		cmdBuffer, m_indexBufferSharedData.bufferData->Get(), m_indexBufferSharedData.offset,
		VK_INDEX_TYPE_UINT32
	);
}
}
