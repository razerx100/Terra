#include <VKSemaphore.hpp>
#include <VKThrowMacros.hpp>

VKSemaphore::VKSemaphore(VkDevice device, size_t semaphoreCount)
	: m_deviceRef{ device },
	m_semaphores{ std::deque<VkSemaphore>{semaphoreCount, VK_NULL_HANDLE} } {

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result{};
	for (size_t _ = 0u; _ < semaphoreCount; ++_) {
		VkSemaphore semaphore = m_semaphores.front();
		m_semaphores.pop();

		VK_THROW_FAILED(result,
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore)
		);

		m_semaphores.push(semaphore);
	}
}

VKSemaphore::~VKSemaphore() noexcept {
	while (!std::empty(m_semaphores)) {
		VkSemaphore semaphore = m_semaphores.front();
		m_semaphores.pop();

		vkDestroySemaphore(m_deviceRef, semaphore, nullptr);
	}
}

void VKSemaphore::AdvanceInQueue() noexcept {
	const VkSemaphore semaphore = m_semaphores.front();
	m_semaphores.pop();
	m_semaphores.push(semaphore);
}

VkSemaphore VKSemaphore::GetFrontSemaphore() const noexcept {
	return m_semaphores.front();
}
