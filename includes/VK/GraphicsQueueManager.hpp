#ifndef GRAPHICS_QUEUE_MANAGER_HPP_
#define GRAPHICS_QUEUE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <FenceWrapper.hpp>
#include <memory>

class GraphicsQueueManager {
public:
	GraphicsQueueManager(VkDevice device, VkQueue queue, std::uint32_t bufferCount);

	void SubmitCommandBuffer(
		VkCommandBuffer commandBuffer, VkSemaphore imageSemaphore
	);

	void SubmitCommandBuffer(
		VkCommandBuffer commandBuffer, size_t fenceIndex = 0u
	);

	void SetNextFrameIndex(size_t index) noexcept;
	void ResetFence() const noexcept;
	void WaitForGPU() const noexcept;

private:
	VkQueue m_graphicsQueue;
	std::unique_ptr<FenceWrapper> m_fences;
	size_t m_currentFrameIndex;
};
#endif
