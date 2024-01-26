#include <VKSemaphore.hpp>

VKSemaphore::VKSemaphore(VkDevice device, size_t semaphoreCount)
	: m_deviceRef{ device } {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t _ = 0u; _ < semaphoreCount; ++_)
		m_semaphores.push(CreateSemaphore(device, semaphoreInfo));
}

VKSemaphore::~VKSemaphore() noexcept {
	while (!std::empty(m_semaphores)) {
		VkSemaphore semaphore = m_semaphores.front();
		m_semaphores.pop();

		vkDestroySemaphore(m_deviceRef, semaphore, nullptr);
	}
}

VkSemaphore VKSemaphore::CreateSemaphore(
	VkDevice device, const VkSemaphoreCreateInfo& createInfo
) const noexcept {
	VkSemaphore semaphore = VK_NULL_HANDLE;
	vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);

	return semaphore;
}

void VKSemaphore::AdvanceInQueue() noexcept {
	const VkSemaphore semaphore = m_semaphores.front();
	m_semaphores.pop();
	m_semaphores.push(semaphore);
}

VkSemaphore VKSemaphore::GetFrontSemaphore() const noexcept {
	return m_semaphores.front();
}
