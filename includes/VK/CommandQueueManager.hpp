#ifndef __COMMAND_QUEUE_MANAGER_HPP__
#define __COMMAND_QUEUE_MANAGER_HPP__
#include <vulkan/vulkan.h>

class GraphicsQueueManager {
public:
	GraphicsQueueManager(VkQueue queue);

	VkQueue GetQueue() const noexcept;

private:
	VkQueue m_graphicsQueue;
};

GraphicsQueueManager* GetGraphicsQueueManagerInstance() noexcept;
void InitGraphicsQueueManagerInstance(VkQueue queue);
void CleanUpGraphicsQueueManagerInstance() noexcept;

#endif
