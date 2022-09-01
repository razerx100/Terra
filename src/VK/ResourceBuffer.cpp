#include <ResourceBuffer.hpp>
#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

ResourceBuffer::ResourceBuffer(
	std::vector<std::uint32_t> queueFamilyIndices, BufferType type
) : m_queueFamilyIndices{ std::move(queueFamilyIndices) }, m_type{ type },
	m_gpuMemoryTypeIndex{ 0u } {

	m_uploadBuffers = std::make_unique<UploadBuffers>();
}

void ResourceBuffer::CreateBuffers(VkDevice device) {
	m_uploadBuffers->CreateBuffers(device);

	VkDeviceMemory gpuOnlyMemory = Terra::Resources::memoryManager->GetMemoryHandle(
		m_gpuMemoryTypeIndex
	);

	VkResult result;
	for (size_t index = 0u; index < std::size(m_gpuBuffers); ++index) {
		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_gpuBuffers[index]->GetBuffer(), gpuOnlyMemory,
				m_gpuBufferData[index].offset
			)
		);
	}
}

std::shared_ptr<GpuBuffer> ResourceBuffer::AddBuffer(
	VkDevice device, std::unique_ptr<std::uint8_t> sourceHandle, size_t bufferSize
) {
	m_uploadBuffers->AddBuffer(device, std::move(sourceHandle), bufferSize);

	std::shared_ptr<GpuBuffer> gpuBuffer = std::make_shared<GpuBuffer>(device);

	gpuBuffer->CreateBuffer(
		device, bufferSize,
		m_queueFamilyIndices, m_type
	);

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device, gpuBuffer->GetBuffer(), &memoryRequirements);

	auto [gpuBufferOffset, gpuMemoryTypeIndex] =
		Terra::Resources::memoryManager->ReserveSizeAndGetMemoryData(
			device, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

	m_gpuBufferData.emplace_back(bufferSize, gpuBufferOffset);

	if (m_gpuMemoryTypeIndex != gpuMemoryTypeIndex)
		m_gpuMemoryTypeIndex = gpuMemoryTypeIndex;

	m_gpuBuffers.emplace_back(gpuBuffer);

	return gpuBuffer;
}

void ResourceBuffer::CopyData() noexcept {
	m_uploadBuffers->CopyData();
}

void ResourceBuffer::RecordUpload(VkDevice device, VkCommandBuffer copyCmdBuffer) {
	m_uploadBuffers->FlushMemory(device);

	const auto& uploadBuffers = m_uploadBuffers->GetUploadBuffers();

	for (size_t index = 0u; index < std::size(m_gpuBufferData); ++index) {
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0u;
		copyRegion.dstOffset = 0u;
		copyRegion.size = static_cast<VkDeviceSize>(m_gpuBufferData[index].bufferSize);

		vkCmdCopyBuffer(
			copyCmdBuffer, uploadBuffers[index]->GetBuffer(),
			m_gpuBuffers[index]->GetBuffer(), 1u, &copyRegion
		);
	}
}

void ResourceBuffer::ReleaseUploadBuffer() noexcept {
	m_gpuBuffers = std::vector<std::shared_ptr<GpuBuffer>>();

	m_uploadBuffers.reset();
}
