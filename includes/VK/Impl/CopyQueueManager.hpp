#ifndef __COPY_QUEUE_MANAGER_HPP__
#define __COPY_QUEUE_MANAGER_HPP__
#include <ICopyQueueManager.hpp>
#include <IFenceWrapper.hpp>
#include <memory>

class CopyQueueManager : public ICopyQueueManager {
public:
	CopyQueueManager(VkDevice device, VkQueue queue);

	void SubmitCommandBuffer(VkCommandBuffer commandBuffer) override;
	void WaitForGPU() override;

private:
	VkQueue m_copyQueue;
	std::unique_ptr<IFenceWrapper> m_fence;
};
#endif
