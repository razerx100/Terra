#include <PerFrameBuffers.hpp>

#include <Terra.hpp>

PerFrameBuffers::PerFrameBuffers(VkDevice device) {
	InitBuffers(device);
}

void PerFrameBuffers::InitBuffers(VkDevice device) {
	size_t bufferSize = sizeof(DirectX::XMMATRIX) * 2u;

	m_pCameraBuffer = Terra::uniformBuffer->AddBuffer(
		device, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
	);

	AddDescriptorForBuffer(
		m_pCameraBuffer->GetBuffer(), bufferSize, 0u,
		VK_SHADER_STAGE_VERTEX_BIT
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

void PerFrameBuffers::UpdatePerFrameBuffers() const noexcept {
	std::uint8_t* cameraHandle = m_pCameraBuffer->GetCpuHandle();

	Terra::cameraManager->CopyData(cameraHandle);
}
