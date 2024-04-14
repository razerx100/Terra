#include <VertexManagerVertexShader.hpp>

VertexManagerVertexShader::VertexManagerVertexShader(
	VkDevice device, MemoryManager* memoryManager
) noexcept
	: m_vertexBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_indexBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_vertices{}, m_indices{}
{}

void VertexManagerVertexShader::SetVerticesAndIndices(
	std::vector<Vertex>&& vertices, std::vector<std::uint32_t>&& indices,
	StagingBufferManager& stagingBufferMan
) noexcept {
	// Vertex Buffer
	{
		const auto vertexBufferSize = static_cast<VkDeviceSize>(sizeof(Vertex) * std::size(vertices));

		m_vertexBuffer.Create(
			static_cast<VkDeviceSize>(vertexBufferSize),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
		);

		m_vertices = std::move(vertices);

		stagingBufferMan.AddBuffer(
			reinterpret_cast<std::uint8_t*>(std::data(m_vertices)), vertexBufferSize, m_vertexBuffer, 0u,
			QueueType::GraphicsQueue, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
			VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT
		);
	}

	// Index Buffer
	{
		const auto indexBufferSize = sizeof(std::uint32_t) * std::size(indices);

		m_indexBuffer.Create(
			static_cast<VkDeviceSize>(indexBufferSize),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
		);

		m_indices = std::move(indices);

		stagingBufferMan.AddBuffer(
			reinterpret_cast<std::uint8_t*>(std::data(m_indices)), indexBufferSize, m_indexBuffer, 0u,
			QueueType::GraphicsQueue, VK_ACCESS_2_INDEX_READ_BIT,
			VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT
		);
	}
}

void VertexManagerVertexShader::BindVertexAndIndexBuffer(
	VkCommandBuffer graphicsCmdBuffer
) const noexcept {
	VkBuffer vertexBuffers[]                  = { m_vertexBuffer.Get() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(graphicsCmdBuffer, 0u, 1u, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(
		graphicsCmdBuffer, m_indexBuffer.Get(), 0u, VK_INDEX_TYPE_UINT32
	);
}

void VertexManagerVertexShader::CleanupTempData() noexcept
{
	m_vertices = std::vector<Vertex>{};
	m_indices  = std::vector<std::uint32_t>{};
}
