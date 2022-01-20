#include <FenceWrapper.hpp>
#include <VKThrowMacros.hpp>

FenceWrapper::FenceWrapper(VkDevice device, size_t bufferCount, bool signaled)
	:
	m_deviceRef(device), m_fences(bufferCount) {

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u;

	VkResult result;
	for (size_t index = 0u; index < bufferCount; ++index) {
		VK_THROW_FAILED(result,
			vkCreateFence(device, &fenceInfo, nullptr, &m_fences[index])
		);
	}
}

FenceWrapper::~FenceWrapper() noexcept {
	for (size_t index = 0u; index < m_fences.size(); ++index)
		vkDestroyFence(m_deviceRef, m_fences[index], nullptr);
}

VkFence FenceWrapper::GetFence(size_t index) const noexcept {
	return m_fences[index];
}

void FenceWrapper::WaitAndResetFence(size_t index) const noexcept {
	vkWaitForFences(m_deviceRef, 1u, &m_fences[index], VK_TRUE, UINT64_MAX);
	vkResetFences(m_deviceRef, 1u, &m_fences[index]);
}
