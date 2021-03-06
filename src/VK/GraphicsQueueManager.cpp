#include <GraphicsQueueManager.hpp>
#include <VKThrowMacros.hpp>

GraphicsQueueManager::GraphicsQueueManager(
	VkDevice device, VkQueue queue, std::uint32_t bufferCount
)	: m_graphicsQueue(queue), m_currentFrameIndex(0u) {
	m_fences = std::make_unique<FenceWrapper>(device, bufferCount, true);
}

void GraphicsQueueManager::SubmitCommandBuffer(
	VkCommandBuffer commandBuffer,
	VkSemaphore imageSemaphore
) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = 1u;
	submitInfo.pWaitSemaphores = &imageSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkResult result;
	VK_THROW_FAILED(result,
		vkQueueSubmit(m_graphicsQueue, 1u, &submitInfo, m_fences->GetFence(m_currentFrameIndex))
	);
}

void GraphicsQueueManager::SubmitCommandBuffer(
	VkCommandBuffer commandBuffer,
	size_t fenceIndex
) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkResult result;
	VK_THROW_FAILED(result,
		vkQueueSubmit(m_graphicsQueue, 1u, &submitInfo, m_fences->GetFence(fenceIndex))
	);
}

void GraphicsQueueManager::SetNextFrameIndex(size_t index) noexcept {
	m_currentFrameIndex = index;
}

void GraphicsQueueManager::WaitForGPU() const noexcept {
	m_fences->WaitForFence(m_currentFrameIndex);
}

void GraphicsQueueManager::ResetFence() const noexcept {
	m_fences->ResetFence(m_currentFrameIndex);
}
