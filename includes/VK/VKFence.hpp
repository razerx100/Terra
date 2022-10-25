#ifndef VK_FENCE_HPP_
#define VK_FENCE_HPP_
#include <vulkan/vulkan.hpp>
#include <queue>

class VKFence {
public:
	VKFence(VkDevice device, size_t fenceCount, bool signaled);
	~VKFence() noexcept;

	void WaitForFrontFence() const noexcept;
	void ResetFrontFence() const noexcept;
	void AdvanceInQueue() noexcept;

	[[nodiscard]]
	VkFence GetFrontFence() const noexcept;

private:
	[[nodiscard]]
	VkFence CreateFence(
		VkDevice device, const VkFenceCreateInfo& createInfo
	) const noexcept;

private:
	VkDevice m_deviceRef;
	std::queue<VkFence> m_fences;
};
#endif
