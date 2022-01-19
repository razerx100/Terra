#include <SyncObjects.hpp>
#include <VKThrowMacros.hpp>

SyncObjects::SyncObjects(VkDevice device, size_t bufferCount)
	:
	m_deviceRef(device), m_imageAvailable(bufferCount), m_renderFinished(bufferCount),
	m_fences(bufferCount),
	m_inFlightFramesLimit(bufferCount), m_frameIndex(0u) {

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result;
	for (size_t index = 0u; index < bufferCount; ++index) {
		VK_THROW_FAILED(result,
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailable[index])
		);

		VK_THROW_FAILED(result,
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinished[index])
		);

		VK_THROW_FAILED(result,
			vkCreateFence(device, &fenceInfo, nullptr, &m_fences[index])
		);
	}
}

SyncObjects::~SyncObjects() noexcept {
	for (size_t index = 0u; index < m_imageAvailable.size(); ++index) {
		vkDestroySemaphore(m_deviceRef, m_imageAvailable[index], nullptr);
		vkDestroySemaphore(m_deviceRef, m_renderFinished[index], nullptr);

		vkDestroyFence(m_deviceRef, m_fences[index], nullptr);
	}
}

VkSemaphore SyncObjects::GetImageAvailableSemaphore() const noexcept {
	return m_imageAvailable[m_frameIndex];
}

VkSemaphore SyncObjects::GetRenderFinishedSemaphore() const noexcept {
	return m_renderFinished[m_frameIndex];
}

VkFence SyncObjects::GetFence() const noexcept {
	return m_fences[m_frameIndex];
}

void SyncObjects::ChangeFrameIndex() noexcept {
	m_frameIndex = (m_frameIndex + 1) % m_inFlightFramesLimit;
}

void SyncObjects::WaitAndResetFence() const noexcept {
	vkWaitForFences(m_deviceRef, 1u, &m_fences[m_frameIndex], VK_TRUE, UINT64_MAX);
	vkResetFences(m_deviceRef, 1u, &m_fences[m_frameIndex]);
}
