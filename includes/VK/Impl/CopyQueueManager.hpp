#ifndef __COPY_QUEUE_MANAGER_HPP__
#define __COPY_QUEUE_MANAGER_HPP__
#include <ICopyQueueManager.hpp>
#include <FenceWrapper.hpp>

class CopyQueueManager : public ICopyQueueManager {
public:
	CopyQueueManager(VkDevice device, VkQueue queue);

	void SubmitCommandBuffer(VkCommandBuffer commandBuffer) override;
	void WaitForGPU() override;

private:
	VkQueue m_copyQueue;
	FenceWrapper m_fence;
};
#endif
