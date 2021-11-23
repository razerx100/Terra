#include <DisplayManagerVK.hpp>

void DisplayManagerVK::GetDisplayResolution(VkPhysicalDevice gpu, SRect& displayRect) {
	std::uint32_t displayCount;
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, nullptr);

	std::vector<VkDisplayPropertiesKHR> displayProperties(displayCount);
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, displayProperties.data());

	displayRect = {};
	// Only handling the first monitor
	if (displayCount == 1) {
		displayRect.right = displayProperties[0].physicalResolution.width;
		displayRect.bottom = displayProperties[0].physicalResolution.height;
	}
}

const std::vector<const char*>& DisplayManagerVK::GetRequiredExtensions() const noexcept {
	return m_requiredExtensions;
}