#ifndef __SEMAPHORE_WRAPPER_HPP__
#define __SEMAPHORE_WRAPPER_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>

class SemaphoreWrapper {
public:
	SemaphoreWrapper(VkDevice device, size_t count);
	~SemaphoreWrapper() noexcept;

	VkSemaphore GetSemaphore(size_t index) const noexcept;

private:
	VkDevice m_deviceRef;
	std::vector<VkSemaphore> m_semaphores;
};
#endif
