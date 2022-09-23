#include <PerFrameBuffers.hpp>

#include <Terra.hpp>

PerFrameBuffers::PerFrameBuffers(
	VkDevice device, std::vector<std::uint32_t> queueFamilyIndices,
	std::uint32_t bufferCount
) noexcept
	: m_cameraBuffer{ device }, m_gVertexBuffer{ device }, m_gIndexBuffer{ device },
	m_queueFamilyIndices{ std::move(queueFamilyIndices) }, m_bufferCount{ bufferCount } {
	InitBuffers(device);
}

void PerFrameBuffers::InitBuffers(VkDevice device) noexcept {
	size_t bufferSize = sizeof(DirectX::XMMATRIX) * 2u;

	m_cameraBuffer.CreateResource(
		device, bufferSize, m_bufferCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	);

	m_cameraBuffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	AddDescriptorForBuffer(
		m_cameraBuffer, 0u, VK_SHADER_STAGE_VERTEX_BIT
	);
}

void PerFrameBuffers::AddDescriptorForBuffer(
	const VkResourceView& buffer,
	std::uint32_t bindingSlot, VkShaderStageFlagBits shaderStage
) noexcept {
	DescriptorInfo descInfo{};
	descInfo.bindingSlot = bindingSlot;
	descInfo.descriptorCount = 1u;
	descInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	std::vector<VkDescriptorBufferInfo> bufferInfos;

	for (VkDeviceSize index = 0u; index < m_bufferCount; ++index) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer.GetResource();
		bufferInfo.offset = buffer.GetMemoryOffset(index);
		bufferInfo.range = buffer.GetSubAllocationSize();

		bufferInfos.emplace_back(std::move(bufferInfo));
	}

	Terra::descriptorSet->AddSetLayout(descInfo, shaderStage, std::move(bufferInfos));
}

void PerFrameBuffers::BindPerFrameBuffers(
	VkCommandBuffer commandBuffer, size_t frameIndex
) const noexcept {
	std::uint8_t* cpuMemoryStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();

	Terra::cameraManager->CopyData(
		cpuMemoryStart + m_cameraBuffer.GetMemoryOffset(static_cast<VkDeviceSize>(frameIndex))
	);

	VkBuffer vertexBuffers[] = { m_gVertexBuffer.GetGPUResource() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(commandBuffer, 0u, 1u, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(
		commandBuffer, m_gIndexBuffer.GetGPUResource(), 0u, VK_INDEX_TYPE_UINT32
	);
}

void PerFrameBuffers::AddModelInputs(
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

void PerFrameBuffers::BindResourceToMemory(VkDevice device) {
	m_cameraBuffer.BindResourceToMemory(device);
	m_gVertexBuffer.BindResourceToMemory(device);
	m_gIndexBuffer.BindResourceToMemory(device);
}

void PerFrameBuffers::RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept {
	m_gVertexBuffer.RecordCopy(copyCmdBuffer);
	m_gIndexBuffer.RecordCopy(copyCmdBuffer);
}

void PerFrameBuffers::ReleaseUploadResources() noexcept {
	m_gVertexBuffer.CleanUpUploadResource();
	m_gIndexBuffer.CleanUpUploadResource();
}
