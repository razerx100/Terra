#include <SurfaceManager.hpp>

void SurfaceManager::SelfDestruct() noexcept
{
	if (m_instance)
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

std::vector<VkPresentModeKHR> SurfaceManager::GetPresentModes(VkPhysicalDevice device) const noexcept
{
	std::uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	std::vector<VkPresentModeKHR> presentModes{ presentModeCount };
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device, m_surface, &presentModeCount, std::data(presentModes)
	);

	return presentModes;
}

std::vector<VkSurfaceFormatKHR> SurfaceManager::GetSurfaceFormat(VkPhysicalDevice device) const noexcept
{
	std::uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	std::vector<VkSurfaceFormatKHR> surfaceFormats{ formatCount };
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		device, m_surface, &formatCount, std::data(surfaceFormats)
	);

	return surfaceFormats;
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
