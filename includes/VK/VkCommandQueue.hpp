#ifndef VK_COMMAND_QUEUE_HPP_
#define VK_COMMAND_QUEUE_HPP_
#include <vulkan/vulkan.hpp>
#include <VKCommandBuffer.hpp>
#include <optional>

class VkCommandQueue {
public:
	struct Args {
		std::optional<VkQueue> queue;
	};

public:
	VkCommandQueue(const Args& arguments);

	void SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkFence fence) const noexcept;
	void SubmitCommandBuffer(
		VkCommandBuffer commandBuffer, VkFence fence,
		std::uint32_t waitSemaphoreCount, VkSemaphore* waitSemaphores,
		VkPipelineStageFlags* waitStages
	) const noexcept;
	void SubmitCommandBuffer(
		VkCommandBuffer commandBuffer, VkSemaphore signalSemaphore
	) const noexcept;

private:
	VkQueue m_commandQueue;
};
#endif
