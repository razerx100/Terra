#ifndef __COMMAND_QUEUE_MANAGER_HPP__
#define __COMMAND_QUEUE_MANAGER_HPP__
#include <ICommandQueueManager.hpp>

class GraphicsQueueManager : public IGraphicsQueueManager {
public:
	GraphicsQueueManager(VkQueue queue);

	VkQueue GetQueue() const noexcept override;

private:
	VkQueue m_graphicsQueue;
};
#endif
