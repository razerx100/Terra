#ifndef VK_SEMAPHORE_HPP_
#define VK_SEMAPHORE_HPP_
#include <vulkan/vulkan.hpp>
#include <queue>

class VKSemaphore {
public:
	VKSemaphore(VkDevice device, size_t semaphoreCount);
	~VKSemaphore() noexcept;

	void AdvanceInQueue() noexcept;

	[[nodiscard]]
	VkSemaphore GetFrontSemaphore() const noexcept;

private:
	[[nodiscard]]
	VkSemaphore CreateSemaphore(
		VkDevice device, const VkSemaphoreCreateInfo& createInfo
	) const noexcept;

private:
	VkDevice m_deviceRef;
	std::queue<VkSemaphore> m_semaphores;
};
#endif
