#include <ResourceBuffer.hpp>
#include <DeviceMemory.hpp>
#include <CRSMath.hpp>
#include <VKThrowMacros.hpp>

ResourceBuffer::ResourceBuffer(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	std::vector<std::uint32_t> queueFamilyIndices,
	BufferType type
)
	: m_currentOffset(0u), m_queueFamilyIndices(std::move(queueFamilyIndices)), m_type(type) {

	m_uploadBuffers = std::make_unique<UploadBuffers>(
		logicalDevice, physicalDevice
		);

	m_gpuBufferMemory = std::make_unique<DeviceMemory>(
		logicalDevice, physicalDevice, m_queueFamilyIndices, false, type
		);
}

void ResourceBuffer::CreateBuffer(VkDevice device) {
	m_gpuBufferMemory->AllocateMemory(m_currentOffset);

	m_uploadBuffers->CreateBuffers(device);

	VkDeviceMemory gpuOnlyMemory = m_gpuBufferMemory->GetMemoryHandle();

	VkResult result;
	for (size_t index = 0u; index < m_gpuBuffers.size(); ++index) {
		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_gpuBuffers[index]->GetBuffer(), gpuOnlyMemory,
				m_gpuBufferData[index].offset
			)
		);
	}
}

std::shared_ptr<GpuBuffer> ResourceBuffer::AddBuffer(
	VkDevice device, const void* source, size_t bufferSize
) {
	m_gpuBufferData.emplace_back(bufferSize, m_currentOffset);

	m_currentOffset += Ceres::Math::Align(
		bufferSize, m_gpuBufferMemory->GetAlignment()
	);

	m_uploadBuffers->AddBuffer(device, source, bufferSize);

	std::shared_ptr<GpuBuffer> gpuBuffer = std::make_shared<GpuBuffer>(device);

	gpuBuffer->CreateBuffer(
		device, bufferSize,
		m_queueFamilyIndices, m_type
	);

	m_gpuBuffers.emplace_back(gpuBuffer.get());

	return gpuBuffer;
}

void ResourceBuffer::CopyData() noexcept {
	m_uploadBuffers->CopyData();
}

void ResourceBuffer::RecordUpload(VkDevice device, VkCommandBuffer copyCmdBuffer) {
	m_uploadBuffers->FlushMemory(device);

	const auto& uploadBuffers = m_uploadBuffers->GetUploadBuffers();

	for (size_t index = 0u; index < m_gpuBufferData.size(); ++index) {
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
