#include <ranges>
#include <algorithm>

#include <BufferManager.hpp>
#include <Terra.hpp>

BufferManager::BufferManager(
	VkDevice device, std::vector<std::uint32_t> queueFamilyIndices,
	std::uint32_t bufferCount
) noexcept
	: m_cameraBuffer{ device }, m_gVertexBuffer{ device }, m_gIndexBuffer{ device },
	m_queueFamilyIndices{ std::move(queueFamilyIndices) }, m_modelBuffers{ device },
	m_bufferCount { bufferCount } {}

void BufferManager::CreateBuffers(VkDevice device) noexcept {
	static constexpr size_t cameraBufferSize = sizeof(DirectX::XMMATRIX) * 2u;

	m_cameraBuffer.CreateResource(
		device, cameraBufferSize, m_bufferCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	);

	m_cameraBuffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	DescriptorSetManager::AddDescriptorForBuffer(
		m_cameraBuffer, m_bufferCount, 0u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT
	);

	const size_t modelCount = std::size(m_opaqueModels);
	const size_t modelBufferSize = sizeof(ModelConstantBuffer) * modelCount;

	m_modelBuffers.CreateResource(
		device, static_cast<VkDeviceSize>(modelBufferSize), m_bufferCount,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);
	m_modelBuffers.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	DescriptorSetManager::AddDescriptorForBuffer(
		m_modelBuffers, m_bufferCount, 2u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT
	);
}

void BufferManager::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void BufferManager::Update(VkDeviceSize index) const noexcept {
	std::uint8_t* cpuMemoryStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();

	Terra::cameraManager->CopyData(cpuMemoryStart + m_cameraBuffer.GetMemoryOffset(index));

	UpdateModelData(index);
}

void BufferManager::BindVertexBuffer(VkCommandBuffer graphicsCommandBuffer) const noexcept {
	VkBuffer vertexBuffers[] = { m_gVertexBuffer.GetGPUResource() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(graphicsCommandBuffer, 0u, 1u, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(
		graphicsCommandBuffer, m_gIndexBuffer.GetGPUResource(), 0u, VK_INDEX_TYPE_UINT32
	);
}

void BufferManager::AddModelInputs(
	VkDevice device,
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	// Vertex Buffer
	m_gVertexBuffer.CreateResource(
		device, static_cast<VkDeviceSize>(vertexBufferSize),
		1u, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_queueFamilyIndices
	);

	m_gVertexBuffer.SetMemoryOffsetAndType(device);

	Terra::Resources::uploadContainer->AddMemory(
		std::move(vertices), vertexBufferSize, m_gVertexBuffer.GetUploadMemoryOffset()
	);

	// Index Buffer
	m_gIndexBuffer.CreateResource(
		device, static_cast<VkDeviceSize>(indexBufferSize),
		1u, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_queueFamilyIndices
	);

	m_gIndexBuffer.SetMemoryOffsetAndType(device);

	Terra::Resources::uploadContainer->AddMemory(
		std::move(indices), indexBufferSize, m_gIndexBuffer.GetUploadMemoryOffset()
	);
}

void BufferManager::UpdateModelData(VkDeviceSize index) const noexcept {
	size_t offset = 0u;
	constexpr size_t bufferStride = sizeof(ModelConstantBuffer);

	std::uint8_t* cpuWriteStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();
	const VkDeviceSize modelBuffersOffset = m_modelBuffers.GetMemoryOffset(index);

	for (auto& model : m_opaqueModels) {
		ModelConstantBuffer modelBuffer{};
		modelBuffer.textureIndex = model->GetTextureIndex();
		modelBuffer.uvInfo = model->GetUVInfo();
		modelBuffer.modelMatrix = model->GetModelMatrix();

		memcpy(cpuWriteStart + modelBuffersOffset + offset, &modelBuffer, bufferStride);

		offset += bufferStride;
	}
}

void BufferManager::BindResourceToMemory(VkDevice device) {
	m_modelBuffers.BindResourceToMemory(device);
	m_cameraBuffer.BindResourceToMemory(device);
	m_gVertexBuffer.BindResourceToMemory(device);
	m_gIndexBuffer.BindResourceToMemory(device);
}

void BufferManager::RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept {
	m_gVertexBuffer.RecordCopy(copyCmdBuffer);
	m_gIndexBuffer.RecordCopy(copyCmdBuffer);
}

void BufferManager::ReleaseUploadResources() noexcept {
	m_gVertexBuffer.CleanUpUploadResource();
	m_gIndexBuffer.CleanUpUploadResource();
}
