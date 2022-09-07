#include <VkResourceViews.hpp>
#include <VKThrowMacros.hpp>

// Vk Resource view
VkResourceView::VkResourceView(VkDevice device) noexcept
	: m_resource{ device }, m_cpuWPtr{ nullptr } {}

void VkResourceView::CreateResource(
	VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
	std::vector<std::uint32_t> queueFamilyIndices
) {
	m_resource.CreateResource(device, bufferSize, usageFlags, queueFamilyIndices);
}

void VkResourceView::BindBufferToMemory(
	VkDevice device, VkDeviceMemory memory, VkDeviceSize offset
) {
	VkResult result{};
	VK_THROW_FAILED(result,
		vkBindBufferMemory(
			device, m_resource.GetResource(),
			memory, offset
		)
	);
}

void VkResourceView::SetCPUWPtr(std::uint8_t* ptr) noexcept {
	m_cpuWPtr = ptr;
}

VkBuffer VkResourceView::GetResource() const noexcept {
	return m_resource.GetResource();
}

std::uint8_t* VkResourceView::GetCPUWPtr() const noexcept {
	return m_cpuWPtr;
}
