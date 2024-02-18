#ifndef	STAGING_BUFFER_MANAGER_HPP_
#define STAGING_BUFFER_MANAGER_HPP_
#include <VkResources.hpp>

class StagingBufferManager
{
public:
private:
	VkDevice       m_device;
	MemoryManager* m_memoryManager;
};
#endif
