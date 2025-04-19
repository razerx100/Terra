#include <DisplayManagerVK.hpp>
#include <cassert>

void DisplayInstanceExtensionVk::SetInstanceExtensions(
	VkInstanceExtensionManager& extensionManager
) noexcept {
	extensionManager.AddExtension(InstanceExtension::VkKhrDisplay);
}

// Display Manager
VkExtent2D DisplayManagerVK::GetDisplayResolution(
	VkPhysicalDevice gpu, std::uint32_t displayIndex
) const {
	std::uint32_t displayCount = 0u;
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, nullptr);

	std::vector<VkDisplayPropertiesKHR> displayProperties{ displayCount };
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, std::data(displayProperties));

	assert(displayCount > displayIndex && "Invalid display index.");

	auto [width, height] = displayProperties[static_cast<size_t>(displayIndex)].physicalResolution;

	return VkExtent2D{ .width = width, .height = height };
}
