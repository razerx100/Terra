#ifndef __FENCE_WRAPPER_HPP__
#define __FENCE_WRAPPER_HPP__
#include <IFenceWrapper.hpp>
#include <vector>

class FenceWrapper : public IFenceWrapper {
public:
	FenceWrapper(VkDevice device, size_t bufferCount, bool signaled);
	~FenceWrapper() noexcept;

	VkFence GetFence(size_t index) const noexcept override;

	void WaitAndResetFence(size_t index) const noexcept override;

private:
	VkDevice m_deviceRef;
	std::vector<VkFence> m_fences;
};
#endif
