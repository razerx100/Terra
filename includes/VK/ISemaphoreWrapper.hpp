#ifndef __I_SEMAPHORE_WRAPPER_HPP__
#define __I_SEMAPHORE_WRAPPER_HPP__
#include <vulkan/vulkan.hpp>

class ISemaphoreWrapper {
public:
	virtual ~ISemaphoreWrapper() = default;

	virtual VkSemaphore GetSemaphore(size_t index) const noexcept = 0;
};

ISemaphoreWrapper* CreateSemaphoreWrapperInstance(
	VkDevice device, size_t bufferCount
);
#endif
