#include <MeshManagerVertexShader.hpp>

MeshManagerVertexShader::MeshManagerVertexShader()
	: m_vertexBufferSharedData{ nullptr, 0u, 0u }, m_indexBufferSharedData{ nullptr, 0u, 0u },
	m_meshBounds{}
{}

void MeshManagerVertexShader::SetMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	SharedBuffer& vertexSharedBuffer, SharedBuffer& indexSharedBuffer,
	std::deque<TempData>& tempDataContainer
) {
	m_meshBounds       = meshBundle->GetBounds();
	TempData& tempData = tempDataContainer.emplace_back(
		TempData{ std::move(meshBundle) }
	);

	// Vertex Buffer
	{
		const std::vector<Vertex>& vertices = tempData.meshBundle->GetVertices();
		const auto vertexBufferSize
			= static_cast<VkDeviceSize>(sizeof(Vertex) * std::size(vertices));

		m_vertexBufferSharedData = vertexSharedBuffer.AllocateAndGetSharedData(vertexBufferSize);

		stagingBufferMan.AddBuffer(
			std::data(vertices), vertexBufferSize,
			m_vertexBufferSharedData.bufferData, m_vertexBufferSharedData.offset,
			QueueType::GraphicsQueue, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
			VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT
		);
	}

	// Index Buffer
	{
		const std::vector<std::uint32_t>& indices = tempData.meshBundle->GetIndices();
		const auto indexBufferSize = sizeof(std::uint32_t) * std::size(indices);

		m_indexBufferSharedData = indexSharedBuffer.AllocateAndGetSharedData(indexBufferSize);

		stagingBufferMan.AddBuffer(
			std::data(indices), indexBufferSize,
			m_indexBufferSharedData.bufferData, m_indexBufferSharedData.offset,
			QueueType::GraphicsQueue, VK_ACCESS_2_INDEX_READ_BIT, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT
		);
	}
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
