#include <VkExternalResourceFactory.hpp>

size_t VkExternalResourceFactory::CreateExternalBuffer(ExternalBufferType type)
{
	VkMemoryPropertyFlagBits memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkBufferUsageFlags usageFlags
			= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (type == ExternalBufferType::CPUVisibleSSBO)
	{
		memoryType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	else if (type == ExternalBufferType::CPUVisibleUniform)
	{
		memoryType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}

	return m_externalBuffers.Add(
		std::make_shared<VkExternalBuffer>(m_device, m_memoryManager, memoryType, usageFlags)
	);
}

void VkExternalResourceFactory::RemoveExternalBuffer(size_t index) noexcept
{
	m_externalBuffers.RemoveElement(index);
}
