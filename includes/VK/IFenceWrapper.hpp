#ifndef __I_FENCE_WRAPPER_HPP__
#define __I_FENCE_WRAPPER_HPP__
#include <vulkan/vulkan.hpp>

class IFenceWrapper {
public:
	virtual ~IFenceWrapper() = default;

	[[nodiscard]]
	virtual VkFence GetFence(size_t index) const noexcept = 0;

	virtual void WaitAndResetFence(size_t index) const noexcept = 0;
};

IFenceWrapper* CreateFenceWrapperInstance(
	VkDevice device, size_t bufferCount, bool signaled
);
#endif
