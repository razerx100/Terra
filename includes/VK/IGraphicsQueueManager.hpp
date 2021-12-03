#ifndef __I_GRAPHICS_QUEUE_MANAGER_HPP__
#define __I_GRAPHICS_QUEUE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <cstdint>

class IGraphicsQueueManager {
public:
	virtual ~IGraphicsQueueManager() = default;

	virtual void SubmitCommandBuffer(VkCommandBuffer commandBuffer) = 0;
};

IGraphicsQueueManager* CreateGraphicsQueueManagerInstance(VkQueue queue);
#endif