#include <GraphicsQueueManager.hpp>

GraphicsQueueManager::GraphicsQueueManager(VkQueue queue)
	: m_graphicsQueue(queue) {}


VkQueue GraphicsQueueManager::GetQueue() const noexcept {
	return m_graphicsQueue;
}
