#include <VkExternalResourceManager.hpp>

VkExternalResourceManager::VkExternalResourceManager(
	VkDevice device, MemoryManager* memoryManager
) : m_resourceFactory{ device, memoryManager }
{}
