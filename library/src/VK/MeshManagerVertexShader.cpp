#include <MeshManagerVertexShader.hpp>
#include <SharedPtrVector.hpp>

MeshManagerVertexShader::MeshManagerVertexShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u }, m_indexBufferSharedData{ nullptr, 0u, 0u },
	m_meshBoundsSharedData{ nullptr, 0u, 0u }
{}

void MeshManagerVertexShader::SetMeshBundle(
	StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer, std::unique_ptr<MeshBundleVS> meshBundle
) {
	// Vertex Buffer
	{
		const std::vector<Vertex>& vertices = meshBundle->GetVertices();
		const auto vertexBufferSize
			= static_cast<VkDeviceSize>(sizeof(Vertex) * std::size(vertices));

		m_vertexBufferSharedData = vertexSharedBuffer.AllocateAndGetSharedData(vertexBufferSize, tempBuffer);

		std::shared_ptr<std::uint8_t> vertexBufferData = CopyVectorToSharedPtr(vertices);

		stagingBufferMan.AddBuffer(
			std::move(vertexBufferData), vertexBufferSize,
			m_vertexBufferSharedData.bufferData, m_vertexBufferSharedData.offset,
			QueueType::GraphicsQueue, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
			VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, tempBuffer
		);
	}

	// Index Buffer
	{
		const std::vector<std::uint32_t>& indices = meshBundle->GetIndices();
		const auto indexBufferSize = sizeof(std::uint32_t) * std::size(indices);

		m_indexBufferSharedData = indexSharedBuffer.AllocateAndGetSharedData(indexBufferSize, tempBuffer);

		std::shared_ptr<std::uint8_t> indexBufferData = CopyVectorToSharedPtr(indices);

		stagingBufferMan.AddBuffer(
			std::move(indexBufferData), indexBufferSize,
			m_indexBufferSharedData.bufferData, m_indexBufferSharedData.offset,
			QueueType::GraphicsQueue, VK_ACCESS_2_INDEX_READ_BIT, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
			tempBuffer
		);
	}
}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer, TemporaryDataBufferGPU& tempBuffer
) {
	SetMeshBundle(stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer, std::move(meshBundle));
}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
	SharedBuffer& boundsSharedBuffer, TemporaryDataBufferGPU& tempBuffer,
	QueueType dstQueue, VkPipelineStageFlagBits2 dstPipelineStage
) {
	const std::vector<MeshBound>& bounds = meshBundle->GetBounds();

	constexpr auto boundStride = sizeof(MeshBound);
	const auto boundSize       = static_cast<VkDeviceSize>(boundStride * std::size(bounds));

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	std::shared_ptr<std::uint8_t> boundBufferData = CopyVectorToSharedPtr(bounds);

	stagingBufferMan.AddBuffer(
		std::move(boundBufferData), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset,
		dstQueue, VK_ACCESS_2_SHADER_READ_BIT, dstPipelineStage, tempBuffer
	);

	SetMeshBundle(stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer, std::move(meshBundle));
}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
	SharedBuffer& boundsSharedBuffer, TemporaryDataBufferGPU& tempBuffer
) {
	const std::vector<MeshBound>& bounds = meshBundle->GetBounds();

	constexpr auto boundStride = sizeof(MeshBound);
	const auto boundSize       = static_cast<VkDeviceSize>(boundStride * std::size(bounds));

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	std::shared_ptr<std::uint8_t> boundBufferData = CopyVectorToSharedPtr(bounds);

	stagingBufferMan.AddBuffer(
		std::move(boundBufferData), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset, tempBuffer
	);

	SetMeshBundle(stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer, std::move(meshBundle));
}

void MeshManagerVertexShader::Bind(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
{
	VkBuffer vertexBuffers[]                  = { m_vertexBufferSharedData.bufferData->Get() };
	static const VkDeviceSize vertexOffsets[] = { m_vertexBufferSharedData.offset };

	VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

	vkCmdBindVertexBuffers(cmdBuffer, 0u, 1u, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(
		cmdBuffer, m_indexBufferSharedData.bufferData->Get(), m_indexBufferSharedData.offset,
		VK_INDEX_TYPE_UINT32
	);
}

MeshManagerVertexShader::BoundsDetails MeshManagerVertexShader::GetBoundsDetails() const noexcept
{
	constexpr auto stride = sizeof(MeshBound);

	return BoundsDetails{
		.offset = static_cast<std::uint32_t>(m_meshBoundsSharedData.offset / stride),
		.count  = static_cast<std::uint32_t>(m_meshBoundsSharedData.size / stride)
	};
}
