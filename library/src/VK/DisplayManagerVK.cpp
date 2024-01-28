#include <DisplayManagerVK.hpp>
#include <cassert>

IDisplayManager::Resolution DisplayManagerVK::GetDisplayResolution(
	VkPhysicalDevice gpu, std::uint32_t displayIndex
) const {
	std::uint32_t displayCount = 0u;
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, nullptr);

	std::vector<VkDisplayPropertiesKHR> displayProperties(displayCount);
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, displayProperties.data());

	assert(displayCount > displayIndex && "Invalid display index.");

	auto [width, height] =
		displayProperties[static_cast<size_t>(displayIndex)].physicalResolution;

	return { width, height };
}
