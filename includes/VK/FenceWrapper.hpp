#ifndef __FENCE_WRAPPER_HPP__
#define __FENCE_WRAPPER_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>

class FenceWrapper {
public:
	FenceWrapper(VkDevice device, size_t bufferCount);
	~FenceWrapper() noexcept;

	VkFence GetFence(size_t index) const noexcept;

	void WaitAndResetFence(size_t index) const noexcept;

private:
	VkDevice m_deviceRef;
	std::vector<VkFence> m_fences;
};
#endif
