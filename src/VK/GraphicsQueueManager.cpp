#include <GraphicsQueueManager.hpp>
#include <VKThrowMacros.hpp>
#include <InstanceManager.hpp>

GraphicsQueueManager::GraphicsQueueManager(VkQueue queue)
	: m_graphicsQueue(queue) {}

void GraphicsQueueManager::SubmitCommandBuffer(VkCommandBuffer commandBuffer) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	ISyncObjects* syncObjectRef = SyncObjInst::GetRef();
	VkSemaphore waitSemaphores[] = { syncObjectRef->GetImageAvailableSemaphore() };

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = std::size(waitSemaphores);
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkSemaphore signalSemaphores[] = { syncObjectRef->GetRenderFinishedSemaphore() };
	submitInfo.signalSemaphoreCount = std::size(signalSemaphores);
	submitInfo.pSignalSemaphores = signalSemaphores;

	VkResult result;
	VK_THROW_FAILED(result,
		vkQueueSubmit(m_graphicsQueue, 1u, &submitInfo, syncObjectRef->GetFence())
	);
}
