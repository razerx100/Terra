#include <VertexManagerVertex.hpp>

#include <Terra.hpp>

VertexManagerVertex::VertexManagerVertex(VkDevice device) noexcept
	: m_gVertexBuffer{ device }, m_gIndexBuffer{ device } {}

void VertexManagerVertex::AddGlobalVertices(
	VkDevice device, std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) noexcept {
	// Vertex Buffer
	m_gVertexBuffer.CreateResource(
		device, static_cast<VkDeviceSize>(vertexBufferSize), 1u,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
	);

	m_gVertexBuffer.SetMemoryOffsetAndType(device);

	Terra::Resources::uploadContainer->AddMemory(
		std::move(vertices), vertexBufferSize, m_gVertexBuffer.GetFirstUploadMemoryOffset()
	);

	// Index Buffer
	m_gIndexBuffer.CreateResource(
		device, static_cast<VkDeviceSize>(indexBufferSize), 1u,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);

	m_gIndexBuffer.SetMemoryOffsetAndType(device);

	Terra::Resources::uploadContainer->AddMemory(
		std::move(indices), indexBufferSize, m_gIndexBuffer.GetFirstUploadMemoryOffset()
	);
}

void VertexManagerVertex::BindVertexAndIndexBuffer(
	VkCommandBuffer graphicsCmdBuffer
) const noexcept {
	VkBuffer vertexBuffers[] = { m_gVertexBuffer.GetResource() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(graphicsCmdBuffer, 0u, 1u, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(
		graphicsCmdBuffer, m_gIndexBuffer.GetResource(), 0u, VK_INDEX_TYPE_UINT32
	);
}

void VertexManagerVertex::AcquireOwnerShips(
	VkCommandBuffer graphicsCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept{
	m_gVertexBuffer.AcquireOwnership(
		graphicsCmdBuffer, srcQueueIndex, dstQueueIndex, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
	);
	m_gIndexBuffer.AcquireOwnership(
		graphicsCmdBuffer, srcQueueIndex, dstQueueIndex, VK_ACCESS_INDEX_READ_BIT,
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
	);
}

void VertexManagerVertex::ReleaseOwnerships(
	VkCommandBuffer transferCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_gVertexBuffer.ReleaseOwnerShip(transferCmdBuffer, srcQueueIndex, dstQueueIndex);
	m_gIndexBuffer.ReleaseOwnerShip(transferCmdBuffer, srcQueueIndex, dstQueueIndex);
}

void VertexManagerVertex::RecordCopy(VkCommandBuffer transferCmdBuffer) noexcept {
	m_gVertexBuffer.RecordCopy(transferCmdBuffer);
	m_gIndexBuffer.RecordCopy(transferCmdBuffer);
}

void VertexManagerVertex::ReleaseUploadResources() noexcept {
	m_gVertexBuffer.CleanUpUploadResource();
	m_gIndexBuffer.CleanUpUploadResource();
}

void VertexManagerVertex::BindResourceToMemory(VkDevice device) const noexcept {
	m_gVertexBuffer.BindResourceToMemory(device);
	m_gIndexBuffer.BindResourceToMemory(device);
}
