#include <SwapChainManager.hpp>
#include <VKThrowMacros.hpp>
#include <InstanceManager.hpp>

SwapChainManager::SwapChainManager(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, size_t bufferCount,
	VkQueue presentQueue, size_t queueFamily
) : m_deviceRef(device), m_presentQueue(presentQueue), m_presentFamilyIndex(queueFamily),
	m_currentFrameIndex(0u) {

	bool useless;
	CreateSwapchain(device, swapCapabilities, surface, bufferCount, width, height, useless);
	QueryImages();
	CreateImageViews();

	m_imageSemaphore = std::unique_ptr<ISemaphoreWrapper>(
		CreateSemaphoreWrapperInstance(device, bufferCount)
		);
}

SwapChainManager::~SwapChainManager() noexcept {
	CleanUpSwapchain();
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

	return availableFormats[0u];
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

void SwapChainManager::CreateImageViews() {
	m_swapchainImageViews.resize(m_swapchainImages.size());

	for (size_t index = 0u; index < m_swapchainImageViews.size(); ++index) {
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
			vkCreateImageView(m_deviceRef, &createInfo, nullptr, &m_swapchainImageViews[index])
		);
	}
}

size_t SwapChainManager::GetAvailableImageIndex() const noexcept {
	std::uint32_t imageIndex;
	vkAcquireNextImageKHR(
		m_deviceRef,
		m_swapchain,
		UINT64_MAX,
		m_imageSemaphore->GetSemaphore(m_currentFrameIndex),
		VK_NULL_HANDLE,
		&imageIndex
	);

	return static_cast<size_t>(imageIndex);
}

void SwapChainManager::PresentImage(
	std::uint32_t imageIndex,
	VkSemaphore renderSemaphore
) {
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1u;
	presentInfo.pWaitSemaphores = &renderSemaphore;

	VkSwapchainKHR swapchains[] = { m_swapchain };
	presentInfo.swapchainCount = static_cast<std::uint32_t>(std::size(swapchains));
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VkResult result;
	VK_THROW_FAILED(result,
		vkQueuePresentKHR(m_presentQueue, &presentInfo)
	);
}

void SwapChainManager::GetUndefinedToTransferBarrier(
	size_t imageIndex,
	VkImageMemoryBarrier& transferBarrier
) const noexcept {
	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0u;
	subResourceRange.levelCount = 1u;
	subResourceRange.baseArrayLayer = 0u;
	subResourceRange.layerCount = 1u;

	transferBarrier = {};
	transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	transferBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	transferBarrier.srcQueueFamilyIndex = static_cast<std::uint32_t>(m_presentFamilyIndex);
	transferBarrier.dstQueueFamilyIndex = static_cast<std::uint32_t>(m_presentFamilyIndex);
	transferBarrier.image = m_swapchainImages[imageIndex];
	transferBarrier.subresourceRange = subResourceRange;
}

void SwapChainManager::GetTransferToPresentBarrier(
	size_t imageIndex,
	VkImageMemoryBarrier& presentBarrier
) const noexcept {
	VkImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResourceRange.baseMipLevel = 0u;
	subResourceRange.levelCount = 1u;
	subResourceRange.baseArrayLayer = 0u;
	subResourceRange.layerCount = 1u;

	presentBarrier = {};
	presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	presentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	presentBarrier.srcQueueFamilyIndex = static_cast<std::uint32_t>(m_presentFamilyIndex);
	presentBarrier.dstQueueFamilyIndex = static_cast<std::uint32_t>(m_presentFamilyIndex);
	presentBarrier.image = m_swapchainImages[imageIndex];
	presentBarrier.subresourceRange = subResourceRange;
}

VkImage SwapChainManager::GetImage(size_t imageIndex) const noexcept {
	return m_swapchainImages[imageIndex];
}

void SwapChainManager::ResizeSwapchain(
	std::uint32_t width, std::uint32_t height, bool& formatChanged
) {
	if (width != m_swapchainExtent.width || height != m_swapchainExtent.height) {
		IDeviceManager* deviceManRef = DeviceInst::GetRef();
		vkDeviceWaitIdle(deviceManRef->GetLogicalDevice());

		CleanUpSwapchain();
		CreateSwapchain(
			deviceManRef->GetLogicalDevice(),
			deviceManRef->GetSwapChainInfo(),
			SurfaceInst::GetRef()->GetSurface(),
			m_swapchainImages.size(),
			width, height, formatChanged
		);
		QueryImages();
		CreateImageViews();
	}
}

void SwapChainManager::CreateSwapchain(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	size_t bufferCount, std::uint32_t width, std::uint32_t height,
	bool& formatChanged
) {
	VkSurfaceFormatKHR swapFormat = ChooseSurfaceFormat(swapCapabilities.formats);
	VkPresentModeKHR swapPresentMode = ChoosePresentMode(swapCapabilities.presentModes);
	VkExtent2D swapExtent = ChooseSwapExtent(swapCapabilities.capabilities, width, height);

	if (swapFormat.format != m_swapchainFormat)
		formatChanged = true;

	m_swapchainExtent = swapExtent;
	m_swapchainFormat = swapFormat.format;

	std::uint32_t imageCount = static_cast<std::uint32_t>(bufferCount);
	if (swapCapabilities.capabilities.maxImageCount > 0u
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
}

void SwapChainManager::QueryImages() {
	std::uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_deviceRef, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_deviceRef, m_swapchain, &imageCount, m_swapchainImages.data());
}

void SwapChainManager::CleanUpSwapchain() noexcept {
	for (VkImageView imageView : m_swapchainImageViews)
		vkDestroyImageView(m_deviceRef, imageView, nullptr);

	vkDestroySwapchainKHR(m_deviceRef, m_swapchain, nullptr);
}

VkSemaphore SwapChainManager::GetImageSemaphore() const noexcept {
	return m_imageSemaphore->GetSemaphore(m_currentFrameIndex);
}

void SwapChainManager::SetNextFrameIndex(size_t index) noexcept {
	m_currentFrameIndex = index;
}
