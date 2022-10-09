#include <VKFence.hpp>
#include <VKThrowMacros.hpp>

VKFence::VKFence(VkDevice device, size_t fenceCount, bool signaled)
	: m_deviceRef{ device }, m_fences{ std::deque<VkFence>{fenceCount, VK_NULL_HANDLE} } {

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u;

	VkResult result{};
	for (size_t _ = 0u; _ < fenceCount; ++_) {
		VkFence fence = m_fences.front();
		m_fences.pop();

		VK_THROW_FAILED(result,
			vkCreateFence(device, &fenceInfo, nullptr, &fence)
		);

		m_fences.push(fence);
	}
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
