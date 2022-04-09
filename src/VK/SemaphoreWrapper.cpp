#include <SemaphoreWrapper.hpp>
#include <VKThrowMacros.hpp>

SemaphoreWrapper::SemaphoreWrapper(VkDevice device, size_t count)
	: m_deviceRef(device), m_semaphores(count) {

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkResult result;
	for (size_t index = 0u; index < count; ++index)
		VK_THROW_FAILED(result,
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_semaphores[index])
		);
}

SemaphoreWrapper::~SemaphoreWrapper() noexcept {
	for (const auto semaphore : m_semaphores)
		vkDestroySemaphore(m_deviceRef, semaphore, nullptr);
}

VkSemaphore SemaphoreWrapper::GetSemaphore(size_t index) const noexcept {
	return m_semaphores[index];
}
