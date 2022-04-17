#ifndef COPY_QUEUE_MANAGER_HPP_
#define COPY_QUEUE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <FenceWrapper.hpp>
#include <memory>

class CopyQueueManager {
public:
	CopyQueueManager(VkDevice device, VkQueue queue);

	void SubmitCommandBuffer(VkCommandBuffer commandBuffer);
	void WaitForGPU();

private:
	VkQueue m_copyQueue;
	std::unique_ptr<FenceWrapper> m_fence;
};
#endif
