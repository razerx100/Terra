#include <SurfaceManager.hpp>

void SurfaceManager::SelfDestruct() noexcept
{
	if (m_instance)
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

VkPresentModeKHR SurfaceManager::GetPresentMode(VkPhysicalDevice device) const noexcept
{
	std::uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	std::vector<VkPresentModeKHR> presentModes{ presentModeCount };
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device, m_surface, &presentModeCount, std::data(presentModes)
	);

	for (VkPresentModeKHR mode : presentModes)
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR SurfaceManager::GetSurfaceFormat(VkPhysicalDevice device) const noexcept
{
	std::uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	std::vector<VkSurfaceFormatKHR> surfaceFormats{ formatCount };
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		device, m_surface, &formatCount, std::data(surfaceFormats)
	);

	for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats) {
		bool surfaceCheck =
			surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB
			||
			surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB;
		if (surfaceCheck
			&&
			surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			return surfaceFormat;
	}

	return std::empty(surfaceFormats) ? VkSurfaceFormatKHR{} : surfaceFormats.front();
}

VkSurfaceCapabilitiesKHR SurfaceManager::GetSurfaceCapabilities(VkPhysicalDevice device) const noexcept
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &surfaceCapabilities);

	return surfaceCapabilities;
}

bool SurfaceManager::CanDeviceSupportSurface(VkPhysicalDevice device) const noexcept
{
	std::uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	std::uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	return presentModeCount && formatCount;
}
