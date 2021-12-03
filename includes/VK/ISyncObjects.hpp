#ifndef __I_SYNC_OBJECTS_HPP__
#define __I_SYNC_OBJECTS_HPP__
#include <vulkan/vulkan.hpp>
#include <cstdint>

class ISyncObjects {
public:
	virtual ~ISyncObjects() = default;

	virtual VkSemaphore GetImageAvailableSemaphore() const noexcept = 0;
	virtual VkSemaphore GetRenderFinishedSemaphore() const noexcept = 0;
	virtual VkFence GetFence() const noexcept = 0;
	virtual void ChangeFrameIndex() noexcept = 0;
	virtual void WaitAndResetFence() const noexcept = 0;
};

ISyncObjects* CreateSyncObjectsInstance(VkDevice device, std::uint32_t bufferCount);

#endif
