#include <ResourceBuffer.hpp>
#include <DeviceMemory.hpp>
#include <CRSMath.hpp>
#include <VKThrowMacros.hpp>

ResourceBuffer::ResourceBuffer(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	BufferType type
)
	: m_currentOffset(0u), m_queueFamilyIndices(queueFamilyIndices), m_type(type) {

	m_uploadBuffers = std::make_unique<UploadBuffers>(
		logDevice, phyDevice
		);

	m_gpuBufferMemory = std::make_unique<DeviceMemory>(
		logDevice, phyDevice, queueFamilyIndices, false, type
		);
}

void ResourceBuffer::CreateBuffer(VkDevice device) {
	m_gpuBufferMemory->AllocateMemory(m_currentOffset);

	m_uploadBuffers->CreateBuffers(device);

	VkDeviceMemory gpuOnlyMemory = m_gpuBufferMemory->GetMemoryHandle();

	const std::vector<UploadBufferData>& uploadBufferData = m_uploadBuffers->GetUploadBufferData();

	VkResult result;
	for (size_t index = 0u; index < m_gpuBuffers.size(); ++index) {
		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_gpuBuffers[index]->GetBuffer(), gpuOnlyMemory,
				uploadBufferData[index].offset
			)
		);
	}
}

std::unique_ptr<GpuBuffer> ResourceBuffer::AddBuffer(
	VkDevice device, const void* source, size_t bufferSize
) {
	std::unique_ptr<GpuBuffer> gpuBuffer = std::make_unique<GpuBuffer>(device);

	m_currentOffset += Ceres::Math::Align(
		bufferSize, m_uploadBuffers->GetMemoryAlignment()
	);

	m_uploadBuffers->AddBuffer(device, source, bufferSize);

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

	const std::vector<UploadBufferData>& uploadBufferData = m_uploadBuffers->GetUploadBufferData();

	for (size_t index = 0u; index < uploadBufferData.size(); ++index) {
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0u;
		copyRegion.dstOffset = 0u;
		copyRegion.size = uploadBufferData[index].bufferSize;

		vkCmdCopyBuffer(
			copyCmdBuffer, uploadBufferData[index].buffer->GetBuffer(),
			m_gpuBuffers[index]->GetBuffer(), 1u, &copyRegion
		);
	}
}

void ResourceBuffer::ReleaseUploadBuffer() noexcept {
	m_gpuBuffers = std::vector<GpuBuffer*>();

	m_uploadBuffers.reset();
}
