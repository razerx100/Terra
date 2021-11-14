#include <SwapChainManager.hpp>

SwapChainManager::SwapChainManager(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height
) {
	VkSurfaceFormatKHR swapFormat = ChooseSurfaceFormat(swapCapabilities.formats);
	VkPresentModeKHR swapPresentMode = ChoosePresentMode(swapCapabilities.presentModes);
	VkExtent2D swapExtent = ChooseSwapExtent(swapCapabilities.capabilities, width, height);
}

VkSwapchainKHR SwapChainManager::GetRef() const noexcept {
	return m_swapchain;
}

VkSurfaceFormatKHR SwapChainManager::ChooseSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& availableFormats
) const noexcept {
	for (const VkSurfaceFormatKHR& surfaceFormat : availableFormats)
		if (surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB
			&& surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			return surfaceFormat;

	return availableFormats[0];
}

VkPresentModeKHR SwapChainManager::ChoosePresentMode(
	const std::vector<VkPresentModeKHR>& availableModes
) const noexcept {
	for (const VkPresentModeKHR& mode : availableModes)
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChainManager::ChooseSwapExtent(
	const VkSurfaceCapabilitiesKHR& capabilities,
	std::uint32_t width, std::uint32_t height
) const noexcept {
	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;
	else {
		VkExtent2D extent = {
			width, height
		};

		extent.height = std::clamp(
			extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height
		);
		extent.height = std::clamp(
			extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width
		);

		return extent;
	}
}
