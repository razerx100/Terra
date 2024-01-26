#include <VertexManagerVertexShader.hpp>

#include <Terra.hpp>

VertexManagerVertexShader::VertexManagerVertexShader(VkDevice device) noexcept
	: m_gVertexBuffer{ device }, m_gIndexBuffer{ device } {}

void VertexManagerVertexShader::AddGVerticesAndIndices(
	VkDevice device, std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) noexcept {
	// Vertex Buffer
	const size_t vertexBufferSize = sizeof(Vertex) * std::size(gVertices);

	m_gVertexBuffer.CreateResource(
		device, static_cast<VkDeviceSize>(vertexBufferSize), 1u,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
	);

	m_gVertexBuffer.SetMemoryOffsetAndType(device);

	UploadContainer& uploadContainer = Terra::Get().Res().UploadCont();

	uploadContainer.AddMemory(
		std::data(gVertices), vertexBufferSize, m_gVertexBuffer.GetFirstUploadMemoryOffset()
	);

	m_gVertices = std::move(gVertices);

	// Index Buffer
	const size_t indexBufferSize = sizeof(std::uint32_t) * std::size(gIndices);

	m_gIndexBuffer.CreateResource(
		device, static_cast<VkDeviceSize>(indexBufferSize), 1u,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT
	);

	m_gIndexBuffer.SetMemoryOffsetAndType(device);

	uploadContainer.AddMemory(
		std::data(gIndices), indexBufferSize, m_gIndexBuffer.GetFirstUploadMemoryOffset()
	);

	m_gIndices = std::move(gIndices);
}

void VertexManagerVertexShader::BindVertexAndIndexBuffer(
	VkCommandBuffer graphicsCmdBuffer
) const noexcept {
	VkBuffer vertexBuffers[] = { m_gVertexBuffer.GetResource() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(graphicsCmdBuffer, 0u, 1u, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(
		graphicsCmdBuffer, m_gIndexBuffer.GetResource(), 0u, VK_INDEX_TYPE_UINT32
	);
}

void VertexManagerVertexShader::AcquireOwnerShips(
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

void VertexManagerVertexShader::ReleaseOwnerships(
	VkCommandBuffer transferCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_gVertexBuffer.ReleaseOwnerShip(transferCmdBuffer, srcQueueIndex, dstQueueIndex);
	m_gIndexBuffer.ReleaseOwnerShip(transferCmdBuffer, srcQueueIndex, dstQueueIndex);
}

void VertexManagerVertexShader::RecordCopy(VkCommandBuffer transferCmdBuffer) noexcept {
	m_gVertexBuffer.RecordCopy(transferCmdBuffer);
	m_gIndexBuffer.RecordCopy(transferCmdBuffer);
}

void VertexManagerVertexShader::ReleaseUploadResources() noexcept {
	m_gVertexBuffer.CleanUpUploadResource();
	m_gIndexBuffer.CleanUpUploadResource();

	m_gVertices = std::vector<Vertex>{};
	m_gIndices = std::vector<std::uint32_t>{};
}

void VertexManagerVertexShader::BindResourceToMemory(VkDevice device) const noexcept {
	m_gVertexBuffer.BindResourceToMemory(device);
	m_gIndexBuffer.BindResourceToMemory(device);
}
