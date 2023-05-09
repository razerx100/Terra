#include <VertexManagerMeshShader.hpp>

#include <Terra.hpp>

VertexManagerMeshShader::VertexManagerMeshShader(
	VkDevice device, std::uint32_t bufferCount, QueueIndicesTG queueIndices
) noexcept
	: m_bufferCount{ bufferCount }, m_queueIndices{ queueIndices }, m_vertexBuffer{ device },
	m_vertexIndicesBuffer{ device }, m_primIndicesBuffer{ device } {}

void VertexManagerMeshShader::AddGVerticesAndPrimIndices(
	VkDevice device, std::vector<Vertex>&& gVertices,
	std::vector<std::uint32_t>&& gVerticesIndices, std::vector<std::uint32_t>&& gPrimIndices
) noexcept {
	std::vector<GLSLVertex> glslVertices = TransformVertices(gVertices);

	ConfigureBuffer(device, std::move(glslVertices), m_gVertices, m_vertexBuffer);
	ConfigureBuffer(
		device, std::move(gVerticesIndices), m_gVerticesIndices, m_vertexIndicesBuffer
	);
	ConfigureBuffer(device, std::move(gPrimIndices), m_gPrimIndices, m_primIndicesBuffer);

	AddDescriptors(m_vertexBuffer, 6u);
	AddDescriptors(m_vertexIndicesBuffer, 7u);
	AddDescriptors(m_primIndicesBuffer, 8u);
}

UploadContainer* VertexManagerMeshShader::GetUploadContainer() noexcept {
	return Terra::Resources::uploadContainer.get();
}

void VertexManagerMeshShader::AcquireOwnerShips(VkCommandBuffer graphicsCmdBuffer) noexcept{
	m_vertexBuffer.AcquireOwnership(
		graphicsCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics,
		VK_ACCESS_SHADER_READ_BIT,	VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT
	);
	m_vertexIndicesBuffer.AcquireOwnership(
		graphicsCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics,
		VK_ACCESS_SHADER_READ_BIT,	VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT
	);
	m_primIndicesBuffer.AcquireOwnership(
		graphicsCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics,
		VK_ACCESS_SHADER_READ_BIT,	VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT
	);
}

void VertexManagerMeshShader::ReleaseOwnerships(VkCommandBuffer transferCmdBuffer) noexcept {
	m_vertexBuffer.ReleaseOwnerShip(
		transferCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics
	);
	m_vertexIndicesBuffer.ReleaseOwnerShip(
		transferCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics
	);
	m_primIndicesBuffer.ReleaseOwnerShip(
		transferCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics
	);
}

void VertexManagerMeshShader::RecordCopy(VkCommandBuffer transferCmdBuffer) noexcept {
	m_vertexBuffer.RecordCopy(transferCmdBuffer);
	m_vertexIndicesBuffer.RecordCopy(transferCmdBuffer);
	m_primIndicesBuffer.RecordCopy(transferCmdBuffer);
}

void VertexManagerMeshShader::ReleaseUploadResources() noexcept {
	m_vertexBuffer.CleanUpUploadResource();
	m_vertexIndicesBuffer.CleanUpUploadResource();
	m_primIndicesBuffer.CleanUpUploadResource();

	m_gVertices = std::vector<GLSLVertex>{};
	m_gVerticesIndices = std::vector<std::uint32_t>{};
	m_gPrimIndices = std::vector<std::uint32_t>{};
}

void VertexManagerMeshShader::BindResourceToMemory(VkDevice device) const noexcept {
	m_vertexBuffer.BindResourceToMemory(device);
	m_vertexIndicesBuffer.BindResourceToMemory(device);
	m_primIndicesBuffer.BindResourceToMemory(device);
}

void VertexManagerMeshShader::AddDescriptors(
	VkUploadableBufferResourceView& buffer, std::uint32_t bindingSlot
) const noexcept {
	DescriptorInfo descInfo{
		.bindingSlot = bindingSlot,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	auto bufferInfo = buffer.GetDescBufferInfoSpread(m_bufferCount);

	Terra::graphicsDescriptorSet->AddBuffersSplit(
		descInfo, std::move(bufferInfo), VK_SHADER_STAGE_MESH_BIT_EXT
	);
}

std::vector<VertexManagerMeshShader::GLSLVertex> VertexManagerMeshShader::TransformVertices(
	const std::vector<Vertex>& vertices
) noexcept {
	std::vector<GLSLVertex> glslVertices{ std::size(vertices) };

	for (size_t index = 0u; index < std::size(vertices); ++index) {
		glslVertices[index].position = vertices[index].position;
		glslVertices[index].normal = vertices[index].normal;
		glslVertices[index].uv = vertices[index].uv;
	}

	return glslVertices;
}
