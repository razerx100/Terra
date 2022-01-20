#ifndef __I_COPY_QUEUE_MANAGER_HPP__
#define __I_COPY_QUEUE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>

class ICopyQueueManager {
public:
	virtual ~ICopyQueueManager() = default;

	virtual void SubmitCommandBuffer(VkCommandBuffer commandBuffer) = 0;
	virtual void WaitForGPU() = 0;
};

ICopyQueueManager* CreateCopyQueueManagerInstance(
	VkDevice device,
	VkQueue queue
);
#endif