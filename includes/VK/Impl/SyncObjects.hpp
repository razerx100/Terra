#ifndef __SYNC_OBJECTS_HPP__
#define __SYNC_OBJECTS_HPP__
#include <ISyncObjects.hpp>
#include <vector>
#include <queue>

class SyncObjects : public ISyncObjects {
public:
	SyncObjects(VkDevice device, std::uint32_t bufferCount);
	~SyncObjects() noexcept;

	VkSemaphore GetImageAvailableSemaphore() const noexcept override;
	VkSemaphore GetRenderFinishedSemaphore() const noexcept override;
	VkFence GetFence() const noexcept override;
	void ChangeFrameIndex() noexcept override;

private:
	VkDevice m_deviceRef;
	std::vector<VkSemaphore> m_imageAvailable;
	std::vector<VkSemaphore> m_renderFinished;
	std::vector<VkFence> m_fences;
	std::queue<std::uint32_t> m_frameIndices;
};
#endif
