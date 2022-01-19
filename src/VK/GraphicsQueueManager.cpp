#include <GraphicsQueueManager.hpp>
#include <VKThrowMacros.hpp>
#include <InstanceManager.hpp>

GraphicsQueueManager::GraphicsQueueManager(VkDevice device, VkQueue queue, size_t bufferCount)
	: m_graphicsQueue(queue), m_renderSemaphore(device, bufferCount),
	m_fences(device, bufferCount) {}

void GraphicsQueueManager::SubmitCommandBuffer(
	VkCommandBuffer commandBuffer,
	VkSemaphore imageSemaphore
) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = 1u;
	submitInfo.pWaitSemaphores = &imageSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore renderSemaphore = m_renderSemaphore.GetSemaphore(m_currentFrameIndex);

	submitInfo.signalSemaphoreCount = 1u;
	submitInfo.pSignalSemaphores = &renderSemaphore;

	VkResult result;
	VK_THROW_FAILED(result,
		vkQueueSubmit(m_graphicsQueue, 1u, &submitInfo, m_fences.GetFence(m_currentFrameIndex))
	);
}

VkSemaphore GraphicsQueueManager::GetRenderSemaphore() const noexcept {
	return m_renderSemaphore.GetSemaphore(m_currentFrameIndex);
}

void GraphicsQueueManager::SetNextFrameIndex(size_t index) noexcept {
	m_currentFrameIndex = index;
}

void GraphicsQueueManager::WaitForGPU() {
	m_fences.WaitAndResetFence(m_currentFrameIndex);
}
