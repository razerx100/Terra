#include <SwapChainManager.hpp>
#include <VKThrowMacros.hpp>
#include <Terra.hpp>

SwapChainManager::SwapChainManager(
	const SwapChainManagerCreateInfo& swapCreateInfo,
	VkQueue presentQueue, size_t queueFamilyIndex
) : m_swapchain(VK_NULL_HANDLE), m_deviceRef(swapCreateInfo.device),
	m_swapchainFormat{}, m_swapchainExtent{},
	m_presentQueue(presentQueue), m_presentFamilyIndex(queueFamilyIndex),
	m_currentFrameIndex(0u) {

	bool useless;
	CreateSwapchain(swapCreateInfo, useless);
	QueryImages();
	CreateImageViews();

	m_imageSemaphore = std::make_unique<SemaphoreWrapper>(
			swapCreateInfo.device, swapCreateInfo.bufferCount
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

	const VkSwapchainKHR swapchains[] = { m_swapchain };
	presentInfo.swapchainCount = 1u;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VkResult result;
	VK_THROW_FAILED(result,
		vkQueuePresentKHR(m_presentQueue, &presentInfo)
	);
}

VkFramebuffer SwapChainManager::GetFramebuffer(size_t imageIndex) const noexcept {
	return m_frameBuffers[imageIndex];
}

bool SwapChainManager::ResizeSwapchain(
	std::uint32_t width, std::uint32_t height,
	VkRenderPass renderPass, bool& formatChanged
) {
	if (width != m_swapchainExtent.width || height != m_swapchainExtent.height) {
		VkDevice logicalDevice = Terra::device->GetLogicalDevice();
		vkDeviceWaitIdle(logicalDevice);

		CleanUpSwapchain();

		SwapChainManagerCreateInfo createInfo = {};
		createInfo.device = logicalDevice;
		createInfo.capabilities = Terra::device->GetSwapChainInfo();
		createInfo.surface = Terra::surface->GetSurface();
		createInfo.bufferCount = static_cast<std::uint32_t>(m_swapchainImages.size());
		createInfo.width = width;
		createInfo.height = height;

		CreateSwapchain(createInfo, formatChanged);
		QueryImages();
		CreateImageViews();
		CreateFramebuffers(m_deviceRef, renderPass, width, height);

		return true;
	}
	else
		return false;
}

void SwapChainManager::CreateSwapchain(
	const SwapChainManagerCreateInfo& swapCreateInfo,
	bool& formatChanged
) {
	VkSurfaceFormatKHR swapFormat = ChooseSurfaceFormat(
		swapCreateInfo.capabilities.formats
	);
	VkPresentModeKHR swapPresentMode = ChoosePresentMode(
		swapCreateInfo.capabilities.presentModes
	);
	VkExtent2D swapExtent = ChooseSwapExtent(
		swapCreateInfo.capabilities.surfaceCapabilities,
		swapCreateInfo.width, swapCreateInfo.height
	);

	if (swapFormat.format != m_swapchainFormat)
		formatChanged = true;

	m_swapchainExtent = swapExtent;
	m_swapchainFormat = swapFormat.format;

	std::uint32_t imageCount = swapCreateInfo.bufferCount;
	if (swapCreateInfo.capabilities.surfaceCapabilities.maxImageCount > 0u
		&& swapCreateInfo.capabilities.surfaceCapabilities.maxImageCount < imageCount)
		imageCount = swapCreateInfo.capabilities.surfaceCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = swapCreateInfo.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = swapFormat.format;
	createInfo.imageColorSpace = swapFormat.colorSpace;
	createInfo.imageExtent = swapExtent;
	createInfo.imageArrayLayers = 1u;
	createInfo.imageUsage =
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = swapCreateInfo.capabilities.surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = swapPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateSwapchainKHR(swapCreateInfo.device, &createInfo, nullptr, &m_swapchain)
	);
}

void SwapChainManager::QueryImages() {
	std::uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_deviceRef, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_deviceRef, m_swapchain, &imageCount, m_swapchainImages.data());
}

void SwapChainManager::CleanUpSwapchain() noexcept {
	for (VkFramebuffer framebuffer : m_frameBuffers)
		vkDestroyFramebuffer(m_deviceRef, framebuffer, nullptr);

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

void SwapChainManager::CreateFramebuffers(
	VkDevice device, VkRenderPass renderPass,
	std::uint32_t width, std::uint32_t height
) {
	m_frameBuffers.resize(m_swapchainImageViews.size());

	VkResult result;
	for (size_t index = 0u; index < m_swapchainImageViews.size(); ++index) {
		VkFramebufferCreateInfo frameBufferInfo = {};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = renderPass;
		frameBufferInfo.attachmentCount = 1u;
		frameBufferInfo.pAttachments = &m_swapchainImageViews[index];
		frameBufferInfo.width = width;
		frameBufferInfo.height = height;
		frameBufferInfo.layers = 1u;

		VK_THROW_FAILED(result,
			vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &m_frameBuffers[index])
		);
	}
}
