#ifndef __SYNC_OBJECTS_HPP__
#define __SYNC_OBJECTS_HPP__
#include <ISyncObjects.hpp>
#include <vector>

class SyncObjects : public ISyncObjects {
public:
	SyncObjects(VkDevice device, size_t bufferCount);
	~SyncObjects() noexcept;

	VkSemaphore GetImageAvailableSemaphore() const noexcept override;
	VkSemaphore GetRenderFinishedSemaphore() const noexcept override;
	VkFence GetFence() const noexcept override;
	void ChangeFrameIndex() noexcept override;
	void WaitAndResetFence() const noexcept override;

private:
	VkDevice m_deviceRef;
	std::vector<VkSemaphore> m_imageAvailable;
	std::vector<VkSemaphore> m_renderFinished;
	std::vector<VkFence> m_fences;

	const size_t m_inFlightFramesLimit;
	size_t m_frameIndex;
};
#endif
