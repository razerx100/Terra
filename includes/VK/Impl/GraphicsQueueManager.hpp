#ifndef __GRAPHICS_QUEUE_MANAGER_HPP__
#define __GRAPHICS_QUEUE_MANAGER_HPP__
#include <IGraphicsQueueManager.hpp>
#include <SemaphoreWrapper.hpp>
#include <FenceWrapper.hpp>

class GraphicsQueueManager : public IGraphicsQueueManager {
public:
	GraphicsQueueManager(VkDevice device, VkQueue queue, size_t bufferCount);

	void SubmitCommandBuffer(
		VkCommandBuffer commandBuffer,
		VkSemaphore imageSemaphore
	) override;

	VkSemaphore GetRenderSemaphore() const noexcept override;

	void SetNextFrameIndex(size_t index) noexcept override;
	void WaitForGPU() override;

private:
	VkQueue m_graphicsQueue;
	SemaphoreWrapper m_renderSemaphore;
	FenceWrapper m_fences;
	size_t m_currentFrameIndex;
};
#endif
