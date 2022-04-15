#include <UploadBuffers.hpp>
#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <CRSMath.hpp>

UploadBuffers::UploadBuffers(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice
) : m_deviceRef(logicalDevice), m_createInfo{}, m_cpuHandle(nullptr), m_currentOffset(0u) {

	m_createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	m_createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	m_createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	m_uploadMemory = std::make_unique<DeviceMemory>(
		logicalDevice, physicalDevice, std::vector<std::uint32_t>(), true
		);
}

UploadBuffers::~UploadBuffers() noexcept {
	for (const auto& bufferData : m_uploadBufferData)
		vkDestroyBuffer(m_deviceRef, bufferData.buffer, nullptr);
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
	for (const UploadBufferData& bufferData : m_uploadBufferData)
		VK_THROW_FAILED(result,
			vkBindBufferMemory(device, bufferData.buffer, uploadMemory, bufferData.offset)
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
	UploadBufferData bufferData = { VK_NULL_HANDLE, bufferSize, m_currentOffset };

	m_currentOffset += Ceres::Math::Align(bufferSize, m_uploadMemory->GetAlignment());

	m_createInfo.size = static_cast<VkDeviceSize>(bufferSize);

	VkResult result;
	VK_THROW_FAILED(result, vkCreateBuffer(device, &m_createInfo, nullptr, &bufferData.buffer));

	m_uploadBufferData.emplace_back(bufferData);
	m_dataHandles.emplace_back(data);
}


size_t UploadBuffers::GetMemoryAlignment() const noexcept {
	return m_uploadMemory->GetAlignment();
}

const std::vector<UploadBufferData>& UploadBuffers::GetUploadBufferData() const noexcept {
	return m_uploadBufferData;
}
