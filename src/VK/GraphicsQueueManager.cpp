#include <GraphicsQueueManager.hpp>

GraphicsQueueManager::GraphicsQueueManager(VkQueue queue)
	: m_graphicsQueue(queue) {}

void GraphicsQueueManager::SubmitCommandBuffer(VkCommandBuffer commandBuffer) {

}
