#include <CopyQueueManager.hpp>
#include <VKThrowMacros.hpp>

CopyQueueManager::CopyQueueManager(VkDevice device, VkQueue queue)
	: m_copyQueue(queue) {

	m_fence = std::make_unique<FenceWrapper>(device, 1u, false);
}

void CopyQueueManager::SubmitCommandBuffer(VkCommandBuffer commandBuffer) {
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1u;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkResult result;
	VK_THROW_FAILED(result,
		vkQueueSubmit(m_copyQueue, 1u, &submitInfo, m_fence->GetFence(0u))
	);
}

void CopyQueueManager::WaitForGPU() {
	m_fence->WaitAndResetFence(0u);
}
