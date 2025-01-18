#include <VkExternalResourceFactory.hpp>

size_t VkExternalResourceFactory::CreateExternalBuffer(ExternalBufferType type)
{
	VkMemoryPropertyFlagBits memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkBufferUsageFlags usageFlags
			= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (type == ExternalBufferType::CPUVisible)
	{
		memoryType = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}

	const size_t bufferIndex = std::size(m_externalBuffers);

	m_externalBuffers.emplace_back(
		std::make_shared<VkExternalBuffer>(m_device, m_memoryManager, memoryType, usageFlags)
	);

	return bufferIndex;
}

void VkExternalResourceFactory::RemoveExternalBuffer(size_t index) noexcept
{
	m_externalBuffers.erase(std::next(std::begin(m_externalBuffers), index));
}
