#include <DisplayManagerVK.hpp>

void DisplayManagerVK::InitDisplayManager(VkPhysicalDevice gpu) {}

void DisplayManagerVK::GetDisplayResolution(VkPhysicalDevice gpu, Ceres::Rect& displayRect) {
	std::uint32_t displayCount;
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, nullptr);

	std::vector<VkDisplayPropertiesKHR> displayProperties(displayCount);
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, displayProperties.data());

	displayRect = {};
	// Only handling the first monitor
	if (displayCount == 1u) {
		displayRect.right = displayProperties[0u].physicalResolution.width;
		displayRect.bottom = displayProperties[0u].physicalResolution.height;
	}
}

const std::vector<const char*>& DisplayManagerVK::GetRequiredExtensions() const noexcept {
	return m_requiredExtensions;
}