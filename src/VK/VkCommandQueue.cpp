#include <VkCommandQueue.hpp>

VkCommandQueue::VkCommandQueue(VkQueue queue) noexcept : m_commandQueue{ queue } {}

void VkCommandQueue::SubmitCommandBuffer(
	VkCommandBuffer commandBuffer, VkFence fence
) const noexcept {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_commandQueue, 1u, &submitInfo, fence);
}

void VkCommandQueue::SubmitCommandBufferForRendering(
	VkCommandBuffer commandBuffer, VkFence fence, VkSemaphore semaphore
) const noexcept {
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1u;
	submitInfo.pWaitSemaphores = &semaphore;

	static constexpr VkPipelineStageFlags waitStages[]{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_commandQueue, 1u, &submitInfo, fence);
}
