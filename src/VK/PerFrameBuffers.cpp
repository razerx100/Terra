#include <PerFrameBuffers.hpp>

#include <Terra.hpp>

PerFrameBuffers::PerFrameBuffers(
	VkDevice device, std::vector<std::uint32_t> queueFamilyIndices
) noexcept
	: m_cameraBuffer{ device }, m_gVertexBuffer{ device }, m_gIndexBuffer{ device },
	m_queueFamilyIndices{ std::move(queueFamilyIndices) } {
	InitBuffers(device);
}

void PerFrameBuffers::InitBuffers(VkDevice device) noexcept {
	size_t bufferSize = sizeof(DirectX::XMMATRIX) * 2u;

	m_cameraBuffer.CreateResource(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	const VkDeviceSize cameraBufferOffset =
		Terra::Resources::cpuWriteMemory->ReserveSizeAndGetOffset(
			m_cameraBuffer.GetMemoryRequirements(device)
		);

	m_cameraBuffer.SetMemoryOffset(cameraBufferOffset);

	AddDescriptorForBuffer(
		m_cameraBuffer.GetResource(), bufferSize, 0u, VK_SHADER_STAGE_VERTEX_BIT
	);
}

void PerFrameBuffers::AddDescriptorForBuffer(
	VkBuffer buffer, size_t bufferSize,
	std::uint32_t bindingSlot, VkShaderStageFlagBits shaderStage
) noexcept {
	DescriptorInfo descInfo{};
	descInfo.bindingSlot = bindingSlot;
	descInfo.descriptorCount = 1u;
	descInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	std::vector<VkDescriptorBufferInfo> bufferInfos;

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = 0u;
	bufferInfo.range = static_cast<VkDeviceSize>(bufferSize);

	bufferInfos.emplace_back(std::move(bufferInfo));

	Terra::descriptorSet->AddSetLayoutAndQueueForBinding(
		descInfo, shaderStage, std::move(bufferInfos)
	);
}

void PerFrameBuffers::BindPerFrameBuffers(VkCommandBuffer commandBuffer) const noexcept {
	std::uint8_t* cpuMemoryStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();

	Terra::cameraManager->CopyData(cpuMemoryStart + m_cameraBuffer.GetMemoryOffset());

	VkBuffer vertexBuffers[] = { m_gVertexBuffer.GetGPUResource() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(
		commandBuffer, 0u, 1u, vertexBuffers, vertexOffsets
	);
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
	m_gVertexBuffer.CreateResources(
		device, static_cast<VkDeviceSize>(vertexBufferSize), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		m_queueFamilyIndices
	);

	const VkDeviceSize vertexUploadOffset =
		Terra::Resources::uploadMemory->ReserveSizeAndGetOffset(
			m_gVertexBuffer.GetUploadMemoryRequirements(device)
		);
	const VkDeviceSize vertexGpuOffset =
		Terra::Resources::gpuOnlyMemory->ReserveSizeAndGetOffset(
			m_gVertexBuffer.GetGPUMemoryRequirements(device)
		);

	m_gVertexBuffer.SetMemoryOffset(vertexUploadOffset, vertexGpuOffset);

	Terra::Resources::uploadContainer->AddMemory(
		std::move(vertices), vertexBufferSize, vertexUploadOffset
	);

	// Index Buffer
	m_gIndexBuffer.CreateResources(
		device, static_cast<VkDeviceSize>(indexBufferSize), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		m_queueFamilyIndices
	);

	const VkDeviceSize indexUploadOffset =
		Terra::Resources::uploadMemory->ReserveSizeAndGetOffset(
			m_gIndexBuffer.GetUploadMemoryRequirements(device)
		);
	const VkDeviceSize indexGpuOffset =
		Terra::Resources::gpuOnlyMemory->ReserveSizeAndGetOffset(
			m_gIndexBuffer.GetGPUMemoryRequirements(device)
		);

	m_gIndexBuffer.SetMemoryOffset(indexUploadOffset, indexGpuOffset);

	Terra::Resources::uploadContainer->AddMemory(
		std::move(indices), indexBufferSize, indexUploadOffset
	);
}

void PerFrameBuffers::BindResourceToMemory(
	VkDevice device,
	VkDeviceMemory uploadmemory, VkDeviceMemory cpuWriteMemory, VkDeviceMemory gpuMemory
) {
	m_cameraBuffer.BindResourceToMemory(device, cpuWriteMemory);
	m_gVertexBuffer.BindResourceToMemory(device, uploadmemory, gpuMemory);
	m_gIndexBuffer.BindResourceToMemory(device, uploadmemory, gpuMemory);
}

void PerFrameBuffers::RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept {
	m_gVertexBuffer.RecordCopy(copyCmdBuffer);
	m_gIndexBuffer.RecordCopy(copyCmdBuffer);
}

void PerFrameBuffers::ReleaseUploadResources() noexcept {
	m_gVertexBuffer.CleanUpUploadResource();
	m_gIndexBuffer.CleanUpUploadResource();
}
