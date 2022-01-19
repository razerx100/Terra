#ifndef __I_GRAPHICS_QUEUE_MANAGER_HPP__
#define __I_GRAPHICS_QUEUE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <cstdint>

class IGraphicsQueueManager {
public:
	virtual ~IGraphicsQueueManager() = default;

	virtual void SubmitCommandBuffer(
		VkCommandBuffer commandBuffer,
		VkSemaphore imageSemaphore
	) = 0;

	virtual VkSemaphore GetRenderSemaphore() const noexcept = 0;

	virtual void SetNextFrameIndex(size_t index) noexcept = 0;
	virtual void WaitForGPU() = 0;
};

IGraphicsQueueManager* CreateGraphicsQueueManagerInstance(
	VkDevice device,
	VkQueue queue,
	size_t bufferCount
);
#endif