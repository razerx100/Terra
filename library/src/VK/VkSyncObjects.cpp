#include <VkSyncObjects.hpp>

VkSyncObjects::VkSyncObjects(VkDevice device, std::uint32_t bufferCount, bool signaledFence)
	: m_fences{ device, bufferCount, signaledFence }, m_semaphores{ device, bufferCount } {}

void VkSyncObjects::ResetFrontFence() const noexcept {
	m_fences.ResetFrontFence();
}

void VkSyncObjects::WaitForFrontFence() const noexcept {
	m_fences.WaitForFrontFence();
}

void VkSyncObjects::AdvanceFenceInQueue() noexcept {
	m_fences.AdvanceInQueue();
}

void VkSyncObjects::AdvanceSemaphoreInQueue() noexcept {
	m_semaphores.AdvanceInQueue();
}

void VkSyncObjects::AdvanceSyncObjectsInQueue() noexcept {
	AdvanceFenceInQueue();
	AdvanceSemaphoreInQueue();
}

VkFence VkSyncObjects::GetFrontFence() const noexcept {
	return m_fences.GetFrontFence();
}

VkSemaphore VkSyncObjects::GetFrontSemaphore() const noexcept {
	return m_semaphores.GetFrontSemaphore();
}
