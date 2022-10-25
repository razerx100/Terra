#ifndef VK_COMMAND_QUEUE_HPP_
#define VK_COMMAND_QUEUE_HPP_
#include <vulkan/vulkan.hpp>
#include <VKCommandBuffer.hpp>

class VkCommandQueue {
public:
	VkCommandQueue(VkQueue queue) noexcept;

	void SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkFence fence) const noexcept;
	void SubmitCommandBufferForRendering(
		VkCommandBuffer commandBuffer, VkFence fence, VkSemaphore waitSemaphore
	) const noexcept;

private:
	VkQueue m_commandQueue;
};
#endif
