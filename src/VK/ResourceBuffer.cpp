#include <ResourceBuffer.hpp>
#include <DeviceMemory.hpp>
#include <CRSMath.hpp>
#include <VKThrowMacros.hpp>

ResourceBuffer::ResourceBuffer(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	BufferType type
)
	: m_currentOffset(0u), m_queueFamilyIndices(queueFamilyIndices),
	m_gpuBufferCreateInfo{} {

	m_uploadBuffers = std::make_unique<UploadBuffers>(
		logDevice, phyDevice
		);

	m_gpuBufferMemory = std::make_unique<DeviceMemory>(
		logDevice, phyDevice, queueFamilyIndices, false, type
		);

	m_gpuBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	m_gpuBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (type == BufferType::Vertex)
		m_gpuBufferCreateInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	else if (type == BufferType::Index)
		m_gpuBufferCreateInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	else if (type == BufferType::UniformAndStorage)
		m_gpuBufferCreateInfo.usage |=
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	ConfigureBufferQueueAccess(m_queueFamilyIndices, m_gpuBufferCreateInfo);
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
				device, m_gpuBuffers[index], gpuOnlyMemory, uploadBufferData[index].offset
			)
		);
	}
}

VkBuffer ResourceBuffer::AddBuffer(VkDevice device, const void* source, size_t bufferSize) {
	VkBuffer gpuBuffer = VK_NULL_HANDLE;

	m_currentOffset += Ceres::Math::Align(bufferSize, m_uploadBuffers->GetMemoryAlignment());

	m_uploadBuffers->AddBuffer(device, source, bufferSize);

	m_gpuBufferCreateInfo.size = bufferSize;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateBuffer(device, &m_gpuBufferCreateInfo, nullptr, &gpuBuffer)
	);

	m_gpuBuffers.emplace_back(gpuBuffer);

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
			copyCmdBuffer, uploadBufferData[index].buffer,
			m_gpuBuffers[index], 1u, &copyRegion
		);
	}
}

void ResourceBuffer::ReleaseUploadBuffer() noexcept {
	m_gpuBuffers = std::vector<VkBuffer>();

	m_uploadBuffers.reset();
}
