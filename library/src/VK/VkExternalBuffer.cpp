#include <VkExternalBuffer.hpp>

VkExternalBuffer::VkExternalBuffer(
	VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType,
	VkBufferUsageFlags usageFlags
) : m_buffer{ device, memoryManager, memoryType }, m_usageFlags{ usageFlags }
{}

void VkExternalBuffer::Create(size_t bufferSize)
{
	// For now, let's assume these buffers would only be used in the Graphics queue.
	m_buffer.Create(static_cast<VkDeviceSize>(bufferSize), m_usageFlags, {});
}
