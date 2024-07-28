#include <MeshManagerVertexShader.hpp>

MeshManagerVertexShader::MeshManagerVertexShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u }, m_indexBufferSharedData{ nullptr, 0u, 0u },
	m_meshBoundsSharedData{ nullptr, 0u, 0u }
{}

void MeshManagerVertexShader::SetMeshBundle(
	StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
	TemporaryDataBuffer& tempBuffer, TempData& tempData
) {
	// Vertex Buffer
	{
		const std::vector<Vertex>& vertices = tempData.meshBundle->GetVertices();
		const auto vertexBufferSize
			= static_cast<VkDeviceSize>(sizeof(Vertex) * std::size(vertices));

		m_vertexBufferSharedData = vertexSharedBuffer.AllocateAndGetSharedData(vertexBufferSize, tempBuffer);

		stagingBufferMan.AddBuffer(
			std::data(vertices), vertexBufferSize,
			m_vertexBufferSharedData.bufferData, m_vertexBufferSharedData.offset,
			QueueType::GraphicsQueue, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
			VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, tempBuffer
		);
	}

	// Index Buffer
	{
		const std::vector<std::uint32_t>& indices = tempData.meshBundle->GetIndices();
		const auto indexBufferSize = sizeof(std::uint32_t) * std::size(indices);

		m_indexBufferSharedData = indexSharedBuffer.AllocateAndGetSharedData(indexBufferSize, tempBuffer);

		stagingBufferMan.AddBuffer(
			std::data(indices), indexBufferSize,
			m_indexBufferSharedData.bufferData, m_indexBufferSharedData.offset,
			QueueType::GraphicsQueue, VK_ACCESS_2_INDEX_READ_BIT, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
			tempBuffer
		);
	}
}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
	TemporaryDataBuffer& tempBuffer, std::deque<TempData>& tempDataContainer
) {
	TempData& tempData = tempDataContainer.emplace_back(
		TempData{ std::move(meshBundle) }
	);

	SetMeshBundle(stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer, tempData);
}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
	SharedBuffer& boundsSharedBuffer, TemporaryDataBuffer& tempBuffer,
	std::deque<TempDataBounds>& tempDataContainer,
	QueueType dstQueue, VkPipelineStageFlagBits2 dstPipelineStage
) {
	TempDataBounds& tempDataBounds = tempDataContainer.emplace_back(
		TempDataBounds{
			.tempData = std::move(meshBundle)
		}
	);
	TempData& tempData = tempDataBounds.tempData;

	SetMeshBundle(stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer, tempData);

	const std::vector<MeshBound>& bounds = tempData.meshBundle->GetBounds();

	constexpr auto boundStride = sizeof(MeshBound);
	const auto boundSize       = static_cast<VkDeviceSize>(boundStride * std::size(bounds));

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	stagingBufferMan.AddBuffer(
		std::data(bounds), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset,
		dstQueue, VK_ACCESS_2_SHADER_READ_BIT, dstPipelineStage, tempBuffer
	);
}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
	SharedBuffer& boundsSharedBuffer, TemporaryDataBuffer& tempBuffer,
	std::deque<TempDataBounds>& tempDataContainer
) {
	TempDataBounds& tempDataBounds = tempDataContainer.emplace_back(
		TempDataBounds{
			.tempData = std::move(meshBundle)
		}
	);
	TempData& tempData = tempDataBounds.tempData;

	SetMeshBundle(stagingBufferMan, vertexSharedBuffer, indexSharedBuffer, tempBuffer, tempData);

	const std::vector<MeshBound>& bounds = tempData.meshBundle->GetBounds();

	constexpr auto boundStride = sizeof(MeshBound);
	const auto boundSize       = static_cast<VkDeviceSize>(boundStride * std::size(bounds));

	m_meshBoundsSharedData = boundsSharedBuffer.AllocateAndGetSharedData(boundSize, tempBuffer);

	stagingBufferMan.AddBuffer(
		std::data(bounds), boundSize,
		m_meshBoundsSharedData.bufferData, m_meshBoundsSharedData.offset, tempBuffer
	);
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
