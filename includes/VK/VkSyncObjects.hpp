#ifndef VK_SYNC_OBJECTS_HPP_
#define VK_SYNC_OBJECTS_HPP_
#include <vulkan/vulkan.hpp>
#include <VKFence.hpp>
#include <VKSemaphore.hpp>
#include <optional>

class VkSyncObjects {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<std::uint32_t> bufferCount = 1u;
		std::optional<bool> signaledFence = false;
	};

public:
	VkSyncObjects(const Args& arguments);

	void ResetFrontFence() const noexcept;
	void WaitForFrontFence() const noexcept;
	void AdvanceFenceInQueue() noexcept;
	void AdvanceSemaphoreInQueue() noexcept;
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
