#ifndef __I_COMMAND_QUEUE_MANAGER_HPP__
#define __I_COMMAND_QUEUE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>

class IGraphicsQueueManager {
public:
	virtual ~IGraphicsQueueManager() = default;

	virtual VkQueue GetQueue() const noexcept = 0;
};

IGraphicsQueueManager* GetGraphicsQueueManagerInstance() noexcept;
void InitGraphicsQueueManagerInstance(VkQueue queue);
void CleanUpGraphicsQueueManagerInstance() noexcept;
#endif