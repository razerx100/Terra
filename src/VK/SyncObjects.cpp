#include <SyncObjects.hpp>
#include <VKThrowMacros.hpp>

SyncObjects::SyncObjects(VkDevice device, std::uint32_t bufferCount)
	:
	m_deviceRef(device), m_imageAvailable(bufferCount), m_renderFinished(bufferCount),
	m_fences(bufferCount) {

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result;
	for (std::uint32_t index = 0u; index < bufferCount; ++index) {
		VK_THROW_FAILED(result,
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailable[index])
		);

		VK_THROW_FAILED(result,
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinished[index])
		);

		VK_THROW_FAILED(result,
			vkCreateFence(device, &fenceInfo, nullptr, &m_fences[index])
		);

		m_frameIndices.push(index);
	}
}

SyncObjects::~SyncObjects() noexcept {
	for (std::uint32_t index = 0u; index < m_imageAvailable.size(); ++index) {
		vkDestroySemaphore(m_deviceRef, m_imageAvailable[index], nullptr);

		vkDestroySemaphore(m_deviceRef, m_renderFinished[index], nullptr);
	}
}

VkSemaphore SyncObjects::GetImageAvailableSemaphore() const noexcept {
	return m_imageAvailable[m_frameIndices.front()];
}

VkSemaphore SyncObjects::GetRenderFinishedSemaphore() const noexcept {
	return m_renderFinished[m_frameIndices.front()];
}

VkFence SyncObjects::GetFence() const noexcept {
	return m_fences[m_frameIndices.front()];
}


void SyncObjects::ChangeFrameIndex() noexcept {
	std::uint32_t index = m_frameIndices.front();
	m_frameIndices.pop();
	m_frameIndices.push(index);
}
