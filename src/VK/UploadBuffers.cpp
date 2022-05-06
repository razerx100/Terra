#include <UploadBuffers.hpp>
#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

UploadBuffers::UploadBuffers(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice
) : m_cpuHandle(nullptr), m_currentOffset(0u) {

	m_uploadMemory = std::make_unique<DeviceMemory>(
		logicalDevice, physicalDevice, std::vector<std::uint32_t>(), true
		);
}

void UploadBuffers::CreateBuffers(VkDevice device) {
	m_uploadMemory->AllocateMemory(m_currentOffset);

	VkDeviceMemory uploadMemory = m_uploadMemory->GetMemoryHandle();

	vkMapMemory(
		device, uploadMemory,
		0u, VK_WHOLE_SIZE, 0u,
		reinterpret_cast<void**>(&m_cpuHandle)
	);

	VkResult result;
	for (size_t index = 0u; index < m_buffers.size(); ++index)
		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_buffers[index]->GetBuffer(),
				uploadMemory, m_uploadBufferData[index].offset
			)
		);
}

void UploadBuffers::FlushMemory(VkDevice device) {
	VkDeviceMemory uploadMemory = m_uploadMemory->GetMemoryHandle();

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = uploadMemory;
	memoryRange.offset = 0u;
	memoryRange.size = VK_WHOLE_SIZE;

	VkResult result;
	VK_THROW_FAILED(result, vkFlushMappedMemoryRanges(device, 1u, &memoryRange));

	vkUnmapMemory(device, uploadMemory);
}

void UploadBuffers::CopyData() noexcept {
	for (size_t index = 0u; index < m_uploadBufferData.size(); ++index)
		memcpy(
			m_cpuHandle + m_uploadBufferData[index].offset, m_dataHandles[index],
			m_uploadBufferData[index].bufferSize
		);
}

void UploadBuffers::AddBuffer(VkDevice device, const void* data, size_t bufferSize) {
	m_uploadBufferData.emplace_back(bufferSize, m_currentOffset);
	m_buffers.emplace_back(std::make_unique<UploadBuffer>(device));

	m_currentOffset += Align(bufferSize, m_uploadMemory->GetAlignment());

	m_buffers.back()->CreateBuffer(device, bufferSize);

	m_dataHandles.emplace_back(data);
}

const std::vector<std::unique_ptr<UploadBuffer>>& UploadBuffers::GetUploadBuffers(
) const noexcept {
	return m_buffers;
}

// Upload Buffer Single

UploadBufferSingle::UploadBufferSingle(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice
) : m_pCpuHandle(nullptr), m_bufferSize(0u) {

	m_pBufferMemory = std::make_unique<DeviceMemory>(
		logicalDevice, physicalDevice, std::vector<std::uint32_t>(), true
		);
	m_pBuffer = std::make_unique<UploadBuffer>(logicalDevice);
}

void UploadBufferSingle::CreateBuffer(
	VkDevice device, size_t bufferSize,
	VkBufferUsageFlags bufferFlags
) {
	m_pBuffer->CreateBuffer(device, bufferSize, bufferFlags);
	m_pBufferMemory->AllocateMemory(
		Align(bufferSize, m_pBufferMemory->GetAlignment())
	);
	m_bufferSize = bufferSize;

	VkDeviceMemory uploadMemory = m_pBufferMemory->GetMemoryHandle();

	vkMapMemory(
		device, uploadMemory,
		0u, VK_WHOLE_SIZE, 0u,
		reinterpret_cast<void**>(&m_pCpuHandle)
	);

	VkResult result;
	VK_THROW_FAILED(result,
		vkBindBufferMemory(
			device, m_pBuffer->GetBuffer(),
			uploadMemory, 0u
		)
	);
}

void UploadBufferSingle::FlushMemory(VkDevice device) {
	VkDeviceMemory uploadMemory = m_pBufferMemory->GetMemoryHandle();

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = uploadMemory;
	memoryRange.offset = 0u;
	memoryRange.size = VK_WHOLE_SIZE;

	VkResult result;
	VK_THROW_FAILED(result, vkFlushMappedMemoryRanges(device, 1u, &memoryRange));
}

void UploadBufferSingle::CopyData(const void* data, size_t bufferSize) {
	if (bufferSize > m_bufferSize)
		VK_GENERIC_THROW("Buffer size bigger than size limit.");

	memcpy(m_pCpuHandle, data, bufferSize);
}

VkBuffer UploadBufferSingle::GetBuffer() const noexcept {
	if (m_pBuffer)
		return m_pBuffer->GetBuffer();
	else
		return VK_NULL_HANDLE;
}
