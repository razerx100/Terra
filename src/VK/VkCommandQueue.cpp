#include <VkCommandQueue.hpp>

VkCommandQueue::VkCommandQueue(const Args& arguments)
	: m_commandQueue{ arguments.queue.value() } {}

void VkCommandQueue::SubmitCommandBuffer(
	VkCommandBuffer commandBuffer, VkFence fence
) const noexcept {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_commandQueue, 1u, &submitInfo, fence);
}

void VkCommandQueue::SubmitCommandBuffer(
	VkCommandBuffer commandBuffer, VkFence fence,
	std::uint32_t waitSemaphoreCount, VkSemaphore* waitSemaphores,
	VkPipelineStageFlags* waitStages
) const noexcept {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = waitSemaphoreCount;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_commandQueue, 1u, &submitInfo, fence);
}

void VkCommandQueue::SubmitCommandBuffer(
	VkCommandBuffer commandBuffer, VkSemaphore signalSemaphore
) const noexcept {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.signalSemaphoreCount = 1u;
	submitInfo.pSignalSemaphores = &signalSemaphore;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_commandQueue, 1u, &submitInfo, VK_NULL_HANDLE);
}
