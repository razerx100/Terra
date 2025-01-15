#include <VkExternalResourceFactory.hpp>
#include <VkExternalBuffer.hpp>

std::unique_ptr<ExternalBuffer> VkExternalResourceFactory::CreateExternalBuffer(
	ExternalBufferType type
) const {
	VkMemoryPropertyFlagBits memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkBufferUsageFlags usageFlags
			= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (type == ExternalBufferType::CPUVisible)
	{
		memoryType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}

	return std::make_unique<VkExternalBuffer>(m_device, m_memoryManager, memoryType, usageFlags);
}
