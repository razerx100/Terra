#include <PerFrameBuffers.hpp>

#include <Terra.hpp>

PerFrameBuffers::PerFrameBuffers(VkDevice device) {
	InitBuffers(device);
}

void PerFrameBuffers::InitBuffers(VkDevice device) {
	size_t bufferSize = sizeof(DirectX::XMMATRIX) * 2u;

	m_pCameraBuffer = Terra::uniformBuffer->AddBuffer(device, bufferSize);

	AddDescriptorForBuffer(
		m_pCameraBuffer->GetResource(), bufferSize, 0u, VK_SHADER_STAGE_VERTEX_BIT
	);
}

void PerFrameBuffers::AddDescriptorForBuffer(
	VkBuffer buffer, size_t bufferSize,
	std::uint32_t bindingSlot, VkShaderStageFlagBits shaderStage
) {
	DescriptorInfo descInfo = {};
	descInfo.bindingSlot = bindingSlot;
	descInfo.descriptorCount = 1u;
	descInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	std::vector<VkDescriptorBufferInfo> bufferInfos;

	VkDescriptorBufferInfo bufferInfo = {};
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

	Terra::cameraManager->CopyData(cpuMemoryStart + m_pCameraBuffer->GetMemoryOffset());

	VkBuffer vertexBuffers[] = { m_gVertexBuffer->GetResource() };
	static const VkDeviceSize vertexOffsets[] = { 0u };

	vkCmdBindVertexBuffers(
		commandBuffer, 0u, 1u, vertexBuffers, vertexOffsets
	);
	vkCmdBindIndexBuffer(
		commandBuffer, m_gIndexBuffer->GetResource(), 0u, VK_INDEX_TYPE_UINT32
	);
}

void PerFrameBuffers::AddModelInputs(
	VkDevice device,
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	m_gVertexBuffer = Terra::vertexBuffer->AddBuffer(
		device, std::move(vertices), vertexBufferSize
	);

	m_gIndexBuffer = Terra::indexBuffer->AddBuffer(
		device, std::move(indices), indexBufferSize
	);
}
