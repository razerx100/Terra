#include <cstring>

#include <CameraManager.hpp>

CameraManager::CameraManager(VkDevice device, MemoryManager* memoryManager)
	: m_activeCameraIndex{ 0u }, m_cameraBufferInstanceSize{ 0u },
	m_cameraBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
	m_cameras{}
{}

std::uint32_t CameraManager::SetCamera(std::shared_ptr<Camera> camera) noexcept
{
	const auto index = static_cast<std::uint32_t>(std::size(m_cameras));

	m_cameras.emplace_back(std::move(camera));

	m_activeCameraIndex = index;

	return index;
}

void CameraManager::RemoveCamera(std::uint32_t index) noexcept
{
	m_cameras.erase(std::begin(m_cameras) + index);
}

void CameraManager::CreateBuffer(
	const std::vector<std::uint32_t>& queueIndices, std::uint32_t frameCount
) {
	m_cameraBufferInstanceSize = static_cast<VkDeviceSize>(sizeof(CameraBufferData));

	const VkDeviceSize cameraBufferSize = m_cameraBufferInstanceSize * frameCount;

	m_cameraBuffer.Create(cameraBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, queueIndices);
}

void CameraManager::Update(VkDeviceSize index) const noexcept
{
	std::uint8_t* bufferAddress = m_cameraBuffer.CPUHandle() + m_cameraBufferInstanceSize * index;

	constexpr size_t matrixSize = sizeof(DirectX::XMMATRIX);

	const std::shared_ptr<Camera>& activeCamera = m_cameras.at(m_activeCameraIndex);

	const DirectX::XMMATRIX view = activeCamera->GetViewMatrix();

	memcpy(bufferAddress, &view, matrixSize);

	activeCamera->GetProjectionMatrix(bufferAddress + matrixSize);
}

void CameraManager::SetDescriptorBufferLayoutGraphics(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
	VkShaderStageFlagBits shaderStage
) const noexcept {
	for (auto& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.AddBinding(
			cameraBindingSlot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, shaderStage
		);
}

void CameraManager::SetDescriptorBufferLayoutCompute(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot
) const noexcept {
	for (auto& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.AddBinding(
			cameraBindingSlot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_COMPUTE_BIT
		);
}

void CameraManager::SetDescriptorBufferGraphics(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot
) const {
	for (size_t index = 0u; index < std::size(descriptorBuffers); ++index)
		descriptorBuffers.at(index).SetUniformBufferDescriptor(
			m_cameraBuffer, cameraBindingSlot, 0u,
			index * m_cameraBufferInstanceSize, m_cameraBufferInstanceSize
		);
}

void CameraManager::SetDescriptorBufferCompute(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot
) const {
	for (size_t index = 0u; index < std::size(descriptorBuffers); ++index)
		descriptorBuffers.at(index).SetUniformBufferDescriptor(
			m_cameraBuffer, cameraBindingSlot, 0u,
			index * m_cameraBufferInstanceSize, m_cameraBufferInstanceSize
		);
}
