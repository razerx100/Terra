#include <cstring>

#include <VkCameraManager.hpp>

namespace Terra
{
CameraManager::CameraManager(VkDevice device, MemoryManager* memoryManager)
	: m_activeCameraIndex{ 0u }, m_cameraBufferInstanceSize{ 0u },
	m_cameraBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }
{}

void CameraManager::CreateBuffer(
	const std::vector<std::uint32_t>& queueIndices, std::uint32_t frameCount
) {
	m_cameraBufferInstanceSize          = static_cast<VkDeviceSize>(sizeof(CameraBufferData));

	const VkDeviceSize cameraBufferSize = m_cameraBufferInstanceSize * frameCount;

	m_cameraBuffer.Create(cameraBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
}

void CameraManager::Update(VkDeviceSize index, const Camera& cameraData) const noexcept
{
	std::uint8_t* bufferAddress = m_cameraBuffer.CPUHandle() + m_cameraBufferInstanceSize * index;

	constexpr size_t matrixSize = sizeof(DirectX::XMMATRIX);

	const DirectX::XMMATRIX view = cameraData.GetViewMatrix();

	memcpy(bufferAddress, &view, matrixSize);

	// Projection matrix's address
	bufferAddress += matrixSize;

	cameraData.GetProjectionMatrix(bufferAddress);

	// Frustum's address
	bufferAddress += matrixSize;

	// In the clip space.
	const Frustum viewFrustum        = cameraData.GetViewFrustum(view);

	constexpr size_t frustumDataSize = sizeof(Frustum);

	memcpy(bufferAddress, &viewFrustum, frustumDataSize);

	// View position's address
	bufferAddress += frustumDataSize;

	const DirectX::XMFLOAT3 viewPosition = cameraData.GetCameraPosition();

	memcpy(bufferAddress, &viewPosition, sizeof(DirectX::XMFLOAT3));
}

void CameraManager::SetDescriptorBufferLayoutGraphics(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
	size_t setLayoutIndex, VkShaderStageFlags shaderStage
) const noexcept {
	for (auto& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.AddBinding(
			cameraBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, shaderStage
		);
}

void CameraManager::SetDescriptorBufferLayoutCompute(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
	size_t setLayoutIndex
) const noexcept {
	for (auto& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.AddBinding(
			cameraBindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
}

void CameraManager::SetDescriptorBufferGraphics(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
	size_t setLayoutIndex
) const {
	for (size_t index = 0u; index < std::size(descriptorBuffers); ++index)
		descriptorBuffers[index].SetUniformBufferDescriptor(
			m_cameraBuffer, cameraBindingSlot, setLayoutIndex, 0u,
			index * m_cameraBufferInstanceSize, m_cameraBufferInstanceSize
		);
}

void CameraManager::SetDescriptorBufferCompute(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
	size_t setLayoutIndex
) const {
	for (size_t index = 0u; index < std::size(descriptorBuffers); ++index)
		descriptorBuffers[index].SetUniformBufferDescriptor(
			m_cameraBuffer, cameraBindingSlot, setLayoutIndex, 0u,
			index * m_cameraBufferInstanceSize, m_cameraBufferInstanceSize
		);
}
}
