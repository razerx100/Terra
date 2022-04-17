#ifndef SEMAPHORE_WRAPPER_HPP_
#define SEMAPHORE_WRAPPER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class SemaphoreWrapper {
public:
	SemaphoreWrapper(VkDevice device, size_t count);
	~SemaphoreWrapper() noexcept;

	[[nodiscard]]
	VkSemaphore GetSemaphore(size_t index) const noexcept;

private:
	VkDevice m_deviceRef;
	std::vector<VkSemaphore> m_semaphores;
};
#endif
