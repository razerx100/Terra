#include <SwapChainManager.hpp>
#include <VKThrowMacros.hpp>
#include <ISyncObjects.hpp>

SwapChainManager::SwapChainManager(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	VkQueue presentQueue, std::uint32_t queueFamily
) : m_deviceRef(device), m_presentQueue(presentQueue), m_presentFamilyIndex(queueFamily) {
	VkSurfaceFormatKHR swapFormat = ChooseSurfaceFormat(swapCapabilities.formats);
	VkPresentModeKHR swapPresentMode = ChoosePresentMode(swapCapabilities.presentModes);
	VkExtent2D swapExtent = ChooseSwapExtent(swapCapabilities.capabilities, width, height);

	m_swapchainExtent = swapExtent;
	m_swapchainFormat = swapFormat.format;

	std::uint32_t imageCount = bufferCount;
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
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = swapCapabilities.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = swapPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result;
	VK_THROW_FAILED(result, vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_swapchain));

	ProcessImages(device);

	CreateImageViews(device);
}

SwapChainManager::~SwapChainManager() noexcept {
	for (VkImageView imageView : m_swapchainImageViews)
		vkDestroyImageView(m_deviceRef, imageView, nullptr);

	vkDestroySwapchainKHR(m_deviceRef, m_swapchain, nullptr);
}

VkSwapchainKHR SwapChainManager::GetRef() const noexcept {
	return m_swapchain;
}

VkExtent2D SwapChainManager::GetSwapExtent() const noexcept {
	return m_swapchainExtent;
}

VkFormat SwapChainManager::GetSwapFormat() const noexcept {
	return m_swapchainFormat;
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

void SwapChainManager::CreateImageViews(VkDevice device) {
	m_swapchainImageViews.resize(m_swapchainImages.size());

	for (std::uint32_t index = 0u; index < m_swapchainImageViews.size(); ++index) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapchainImages[index];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapchainFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0u;
		createInfo.subresourceRange.levelCount = 1u;
		createInfo.subresourceRange.baseArrayLayer = 0u;
		createInfo.subresourceRange.layerCount = 1u;

		VkResult result;
		VK_THROW_FAILED(result,
			vkCreateImageView(device, &createInfo, nullptr, &m_swapchainImageViews[index])
		);
	}
}

std::uint32_t SwapChainManager::GetAvailableImageIndex() const noexcept {
	std::uint32_t imageIndex;
	vkAcquireNextImageKHR(
		m_deviceRef,
		m_swapchain,
		UINT64_MAX,
		GetSyncObjectsInstance()->GetImageAvailableSemaphore(),
		VK_NULL_HANDLE,
		&imageIndex
	);

	return imageIndex;
}

void SwapChainManager::PresentImage(std::uint32_t imageIndex) {
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	VkSemaphore signalSemaphores[] = { GetSyncObjectsInstance()->GetRenderFinishedSemaphore() };
	presentInfo.waitSemaphoreCount = std::size(signalSemaphores);
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = { m_swapchain };
	presentInfo.swapchainCount = std::size(swapchains);
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VkResult result;
	VK_THROW_FAILED(result,
		vkQueuePresentKHR(m_presentQueue, &presentInfo)
	);
}

VkImageMemoryBarrier SwapChainManager::GetPresentToRenderBarrier(
	std::uint32_t imageIndex
) const noexcept {
	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1u;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1u;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = m_presentFamilyIndex;
	barrier.dstQueueFamilyIndex = m_presentFamilyIndex;
	barrier.image = m_swapchainImages[imageIndex];
	barrier.subresourceRange = subResourceRange;

	return barrier;
}

VkImageMemoryBarrier SwapChainManager::GetRenderToPresentBarrier(
	std::uint32_t imageIndex
) const noexcept {
	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1u;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1u;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcQueueFamilyIndex = m_presentFamilyIndex;
	barrier.dstQueueFamilyIndex = m_presentFamilyIndex;
	barrier.image = m_swapchainImages[imageIndex];
	barrier.subresourceRange = subResourceRange;

	return barrier;
}

VkImage SwapChainManager::GetImage(std::uint32_t imageIndex) const noexcept {
	return m_swapchainImages[imageIndex];
}

void SwapChainManager::ProcessImages(VkDevice device) {
	std::uint32_t imageCount;
	vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, m_swapchain, &imageCount, m_swapchainImages.data());

	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1u;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1u;

	for (VkImage image : m_swapchainImages) {
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcQueueFamilyIndex = m_presentFamilyIndex;
		barrier.dstQueueFamilyIndex = m_presentFamilyIndex;
		barrier.image = image;
		barrier.subresourceRange = subResourceRange;
	}
}
