#ifndef __GRAPHICS_QUEUE_MANAGER_HPP__
#define __GRAPHICS_QUEUE_MANAGER_HPP__
#include <IGraphicsQueueManager.hpp>

class GraphicsQueueManager : public IGraphicsQueueManager {
public:
	GraphicsQueueManager(VkQueue queue);

	VkQueue GetQueue() const noexcept override;

private:
	VkQueue m_graphicsQueue;
};
#endif