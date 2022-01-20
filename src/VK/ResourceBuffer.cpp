#include <ResourceBuffer.hpp>
#include <VKThrowMacros.hpp>

ResourceBuffer::ResourceBuffer(VkDevice device) noexcept
	: m_deviceRef(device), m_bufferMemory(nullptr), m_cpuHandle(nullptr) {}

ResourceBuffer::~ResourceBuffer() noexcept {
	vkFreeMemory(m_deviceRef, m_bufferMemory, nullptr);
}

void ResourceBuffer::CreateBuffer(
	size_t bufferSize,
	size_t memoryTypeIndex
) {
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = bufferSize;
	allocInfo.memoryTypeIndex = static_cast<std::uint32_t>(memoryTypeIndex);

	VkResult result;
	VK_THROW_FAILED(result,
		vkAllocateMemory(m_deviceRef, &allocInfo, nullptr, &m_bufferMemory)
	);
}

VkDeviceMemory ResourceBuffer::GetGPUHandle() const noexcept {
	return m_bufferMemory;
}

std::uint8_t* ResourceBuffer::GetCPUHandle() const noexcept {
	return m_cpuHandle;
}

void ResourceBuffer::MapCPU() noexcept {
	vkMapMemory(
		m_deviceRef, m_bufferMemory, 0u, VK_WHOLE_SIZE, 0u, reinterpret_cast<void**>(&m_cpuHandle)
	);

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.pNext = nullptr;
	memoryRange.memory = m_bufferMemory;
	memoryRange.offset = 0u;
	memoryRange.size = VK_WHOLE_SIZE;

	vkInvalidateMappedMemoryRanges(m_deviceRef, 1u, &memoryRange);
}

void ResourceBuffer::UnMapCPU() noexcept {
	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.pNext = nullptr;
	memoryRange.memory = m_bufferMemory;
	memoryRange.offset = 0u;
	memoryRange.size = VK_WHOLE_SIZE;

	vkFlushMappedMemoryRanges(m_deviceRef, 1u, &memoryRange);

	vkUnmapMemory(
		m_deviceRef, m_bufferMemory
	);
}
