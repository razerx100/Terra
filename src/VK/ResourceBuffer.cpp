#include <ResourceBuffer.hpp>
#include <DeviceMemory.hpp>
#include <CRSMath.hpp>
#include <VKThrowMacros.hpp>

ResourceBuffer::ResourceBuffer(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	BufferType type
)
	: m_currentOffset(0u), m_cpuHandle(nullptr),
	m_queueFamilyIndices(queueFamilyIndices),
	m_uploadBufferCreateInfo{}, m_gpuBufferCreateInfo{} {

	m_uploadBufferMemory = std::make_unique<DeviceMemory>(
		logDevice, phyDevice, queueFamilyIndices, true
		);
	m_gpuBufferMemory = std::make_unique<DeviceMemory>(
		logDevice, phyDevice, queueFamilyIndices, false, type
		);

	m_uploadBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	m_uploadBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	ConfigureBufferQueueAccess(m_queueFamilyIndices, m_uploadBufferCreateInfo);

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
	m_uploadBufferMemory->AllocateMemory(m_currentOffset);
	m_gpuBufferMemory->AllocateMemory(m_currentOffset);

	VkDeviceMemory uploadMemory = m_uploadBufferMemory->GetMemoryHandle();
	VkDeviceMemory gpuOnlyMemory = m_gpuBufferMemory->GetMemoryHandle();

	VkResult result;
	for (size_t index = 0u; index < m_uploadBufferData.size(); ++index) {
		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_uploadBufferData[index].buffer,
				uploadMemory, m_uploadBufferData[index].offset
			)
		);
		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_gpuBuffers[index], gpuOnlyMemory, m_uploadBufferData[index].offset
			)
		);
	}

	vkMapMemory(
		device, uploadMemory,
		0u, static_cast<VkDeviceSize>(m_currentOffset), 0u,
		reinterpret_cast<void**>(&m_cpuHandle)
	);
}

VkBuffer ResourceBuffer::AddBuffer(VkDevice device, const void* source, size_t bufferSize) {
	BufferData bufferData = { nullptr, source, bufferSize, m_currentOffset };
	VkBuffer gpuBuffer = nullptr;

	m_currentOffset += Ceres::Math::Align(bufferSize, m_uploadBufferMemory->GetAlignment());

	m_uploadBufferCreateInfo.size = bufferSize;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateBuffer(device, &m_uploadBufferCreateInfo, nullptr, &bufferData.buffer)
	);

	m_gpuBufferCreateInfo.size = bufferSize;

	VK_THROW_FAILED(result,
		vkCreateBuffer(device, &m_gpuBufferCreateInfo, nullptr, &gpuBuffer)
	);

	m_uploadBufferData.emplace_back(bufferData);
	m_gpuBuffers.emplace_back(gpuBuffer);

	return gpuBuffer;
}

void ResourceBuffer::CopyData() noexcept {
	for (auto& bufferData : m_uploadBufferData)
		memcpy(m_cpuHandle + bufferData.offset, bufferData.data, bufferData.size);
}

void ResourceBuffer::RecordUpload(VkDevice device, VkCommandBuffer copyCmdBuffer) {
	VkDeviceMemory uploadMemory = m_uploadBufferMemory->GetMemoryHandle();

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.pNext = nullptr;
	memoryRange.memory = uploadMemory;
	memoryRange.offset = 0u;
	memoryRange.size = m_currentOffset;

	VkResult result;
	VK_THROW_FAILED(result,
		vkFlushMappedMemoryRanges(device, 1u, &memoryRange)
	);

	vkUnmapMemory(device, uploadMemory);

	for (size_t index = 0u; index < m_uploadBufferData.size(); ++index) {
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0u;
		copyRegion.dstOffset = 0u;
		copyRegion.size = m_uploadBufferData[index].size;

		vkCmdCopyBuffer(
			copyCmdBuffer, m_uploadBufferData[index].buffer,
			m_gpuBuffers[index], 1u, &copyRegion
		);
	}
}

void ResourceBuffer::ReleaseUploadBuffer(VkDevice device) noexcept {
	m_gpuBuffers = std::vector<VkBuffer>();

	for (auto& buffer : m_uploadBufferData)
		vkDestroyBuffer(device, buffer.buffer, nullptr);

	m_uploadBufferMemory.reset();
}
