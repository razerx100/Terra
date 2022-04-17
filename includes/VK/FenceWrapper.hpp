#ifndef FENCE_WRAPPER_HPP_
#define FENCE_WRAPPER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class FenceWrapper {
public:
	FenceWrapper(VkDevice device, size_t bufferCount, bool signaled);
	~FenceWrapper() noexcept;

	[[nodiscard]]
	VkFence GetFence(size_t index) const noexcept;

	void WaitAndResetFence(size_t index) const noexcept;

private:
	VkDevice m_deviceRef;
	std::vector<VkFence> m_fences;
};
#endif
