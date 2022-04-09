#ifndef __GRAPHICS_QUEUE_MANAGER_HPP__
#define __GRAPHICS_QUEUE_MANAGER_HPP__
#include <IGraphicsQueueManager.hpp>
#include <ISemaphoreWrapper.hpp>
#include <IFenceWrapper.hpp>
#include <memory>

class GraphicsQueueManager : public IGraphicsQueueManager {
public:
	GraphicsQueueManager(VkDevice device, VkQueue queue, size_t bufferCount);

	void SubmitCommandBuffer(
		VkCommandBuffer commandBuffer,
		VkSemaphore imageSemaphore
	) override;

	[[nodiscard]]
	VkSemaphore GetRenderSemaphore() const noexcept override;

	void SetNextFrameIndex(size_t index) noexcept override;
	void WaitForGPU() override;

private:
	VkQueue m_graphicsQueue;
	std::unique_ptr<ISemaphoreWrapper> m_renderSemaphores;
	std::unique_ptr<IFenceWrapper> m_fences;
	size_t m_currentFrameIndex;
};
#endif
