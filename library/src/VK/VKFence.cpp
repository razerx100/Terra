#include <VKFence.hpp>

VKFence::VKFence(VkDevice device, size_t fenceCount, bool signaled)
	: m_deviceRef{ device } {

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0u;

	assert(fenceCount != 0u && "Fence Count can't be zero");

	m_fences.push(CreateFence(device, fenceInfo));
	--fenceCount;

	fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u;

	for (size_t _ = 0u; _ < fenceCount; ++_)
		m_fences.push(CreateFence(device, fenceInfo));
}

VKFence::~VKFence() noexcept {
	while (!std::empty(m_fences)) {
		VkFence fence = m_fences.front();
		m_fences.pop();

		vkDestroyFence(m_deviceRef, fence, nullptr);
	}
}

void VKFence::WaitForFrontFence() const noexcept {
	vkWaitForFences(m_deviceRef, 1u, &m_fences.front(), VK_TRUE, UINT64_MAX);
}

void VKFence::ResetFrontFence() const noexcept {
	vkResetFences(m_deviceRef, 1u, &m_fences.front());
}

void VKFence::AdvanceInQueue() noexcept {
	VkFence fence = m_fences.front();
	m_fences.pop();
	m_fences.push(fence);
}

VkFence VKFence::GetFrontFence() const noexcept {
	return m_fences.front();
}

VkFence VKFence::CreateFence(
	VkDevice device, const VkFenceCreateInfo& createInfo
) const noexcept {
	VkFence fence = VK_NULL_HANDLE;
	vkCreateFence(device, &createInfo, nullptr, &fence);

	return fence;
}
