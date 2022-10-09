#ifndef VK_SYNC_OBJECTS_HPP_
#define VK_SYNC_OBJECTS_HPP_
#include <vulkan/vulkan.hpp>
#include <VKFence.hpp>
#include <VKSemaphore.hpp>

class VkSyncObjects {
public:
	VkSyncObjects(
		VkDevice device, std::uint32_t bufferCount = 1u, bool signaledFence = false
	);

	void ResetFrontFence() const noexcept;
	void WaitForFrontFence() const noexcept;
	void AdvanceSyncObjectsInQueue() noexcept;

	[[nodiscard]]
	VkFence GetFrontFence() const noexcept;
	[[nodiscard]]
	VkSemaphore GetFrontSemaphore() const noexcept;

private:
	VKFence m_fences;
	VKSemaphore m_semaphores;
};
#endif
