#include <DisplayManagerVK.hpp>
#include <VKThrowMacros.hpp>

IDisplayManager::Resolution DisplayManagerVK::GetDisplayResolution(
	VkPhysicalDevice gpu, std::uint32_t displayIndex
) const {
	std::uint32_t displayCount = 0u;
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, nullptr);

	std::vector<VkDisplayPropertiesKHR> displayProperties(displayCount);
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, displayProperties.data());

	if (displayCount <= displayIndex)
		VK_GENERIC_THROW("Searched GPU couldn't be found.");

	auto [width, height] =
		displayProperties[static_cast<size_t>(displayIndex)].physicalResolution;

	return { width, height };
}

const std::vector<const char*>& DisplayManagerVK::GetRequiredExtensions() const noexcept {
	return m_requiredExtensions;
}