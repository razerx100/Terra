#include <SwapChainManager.hpp>
#include <VKThrowMacros.hpp>

SwapChainManager::SwapChainManager(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height
) : m_deviceRef(device) {
	VkSurfaceFormatKHR swapFormat = ChooseSurfaceFormat(swapCapabilities.formats);
	VkPresentModeKHR swapPresentMode = ChoosePresentMode(swapCapabilities.presentModes);
	VkExtent2D swapExtent = ChooseSwapExtent(swapCapabilities.capabilities, width, height);

	m_swapchainExtent = swapExtent;
	m_swapchainFormat = swapFormat.format;

	std::uint32_t imageCount = swapCapabilities.capabilities.minImageCount + 1;
	if (swapCapabilities.capabilities.maxImageCount > 0
		&& swapCapabilities.capabilities.maxImageCount < imageCount)
		imageCount = swapCapabilities.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = swapFormat.format;
	createInfo.imageColorSpace = swapFormat.colorSpace;
	createInfo.imageExtent = swapExtent;
	createInfo.imageArrayLayers = 1u;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = swapCapabilities.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = swapPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result;
	VK_THROW_FAILED(result, vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapchain));

	vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, m_swapchainImages.data());
}

SwapChainManager::~SwapChainManager() noexcept {
	vkDestroySwapchainKHR(m_deviceRef, m_swapchain, nullptr);
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
