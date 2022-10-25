#include <SwapChainManager.hpp>
#include <VkResourceViews.hpp>
#include <VKThrowMacros.hpp>

SwapChainManager::SwapChainManager(
	const SwapChainManagerCreateInfo& swapCreateInfo, VkQueue presentQueue
) : m_swapchain{ VK_NULL_HANDLE }, m_deviceRef{ swapCreateInfo.device },
	m_swapchainFormat{}, m_swapchainExtent{}, m_presentQueue{ presentQueue },
	m_surfaceInfo{ swapCreateInfo.surfaceInfo }, m_nextImageIndex{ 0u } {

	bool useless;
	CreateSwapchain(swapCreateInfo, useless);
	QueryImages();
	CreateImageViews(swapCreateInfo.device);
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
	for (const VkSurfaceFormatKHR& surfaceFormat : availableFormats) {
		bool surfaceCheck =
			surfaceFormat.format == VK_FORMAT_R8G8B8A8_SRGB
			||
			surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB;
		if (surfaceCheck
			&&
			surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			return surfaceFormat;
	}

	return std::empty(availableFormats) ? VkSurfaceFormatKHR() : availableFormats[0u];
}

VkPresentModeKHR SwapChainManager::ChoosePresentMode(
	const std::vector<VkPresentModeKHR>& availableModes
) const noexcept {
	for (const VkPresentModeKHR& mode : availableModes)
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

size_t SwapChainManager::GetNextImageIndex() const noexcept {
	return m_nextImageIndex;
}

void SwapChainManager::CreateImageViews(VkDevice device) {
	m_swapchainImageViews.resize(std::size(m_swapchainImages));

	for (size_t index = 0u; index < std::size(m_swapchainImageViews); ++index)
		VkImageResourceView::_createImageView(
			device, m_swapchainImages[index],
			&m_swapchainImageViews[index], m_swapchainFormat,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
}

void SwapChainManager::AcquireNextImageIndex(VkSemaphore signalSemaphore) noexcept {
	std::uint32_t imageIndex;
	vkAcquireNextImageKHR(
		m_deviceRef, m_swapchain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, &imageIndex
	);

	m_nextImageIndex = imageIndex;
}

void SwapChainManager::PresentImage(std::uint32_t imageIndex) const noexcept {
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	const VkSwapchainKHR swapchains[] = { m_swapchain };
	presentInfo.swapchainCount = 1u;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
}

VkFramebuffer SwapChainManager::GetFramebuffer(size_t imageIndex) const noexcept {
	return m_frameBuffers[imageIndex];
}

void SwapChainManager::ResizeSwapchain(
	VkDevice device, VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height,
	VkRenderPass renderPass, VkImageView depthImageView, bool& formatChanged
) {
	CleanUpSwapchain();

	SwapChainManagerCreateInfo createInfo = {};
	createInfo.device = device;
	createInfo.surfaceInfo = m_surfaceInfo;
	createInfo.surface = surface;
	createInfo.bufferCount = static_cast<std::uint32_t>(std::size(m_swapchainImages));
	createInfo.width = width;
	createInfo.height = height;

	CreateSwapchain(createInfo, formatChanged);
	QueryImages();
	CreateImageViews(device);
	CreateFramebuffers(device, renderPass, depthImageView, width, height);
}

void SwapChainManager::CreateSwapchain(
	const SwapChainManagerCreateInfo& swapCreateInfo,
	bool& formatChanged
) {
	VkSurfaceFormatKHR swapFormat = ChooseSurfaceFormat(
		swapCreateInfo.surfaceInfo.formats
	);
	VkPresentModeKHR swapPresentMode = ChoosePresentMode(
		swapCreateInfo.surfaceInfo.presentModes
	);
	VkExtent2D swapExtent = { swapCreateInfo.width, swapCreateInfo.height };
	m_surfaceInfo.capabilities.currentExtent = swapExtent;

	if (swapFormat.format != m_swapchainFormat)
		formatChanged = true;

	m_swapchainExtent = swapExtent;
	m_swapchainFormat = swapFormat.format;

	std::uint32_t imageCount = swapCreateInfo.bufferCount;
	if (swapCreateInfo.surfaceInfo.capabilities.maxImageCount > 0u
		&& swapCreateInfo.surfaceInfo.capabilities.maxImageCount < imageCount)
		imageCount = swapCreateInfo.surfaceInfo.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = swapCreateInfo.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = swapFormat.format;
	createInfo.imageColorSpace = swapFormat.colorSpace;
	createInfo.imageExtent = swapExtent;
	createInfo.imageArrayLayers = 1u;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = swapCreateInfo.surfaceInfo.capabilities.currentTransform;
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
	vkGetSwapchainImagesKHR(
		m_deviceRef, m_swapchain, &imageCount, std::data(m_swapchainImages)
	);
}

void SwapChainManager::CleanUpSwapchain() noexcept {
	for (VkFramebuffer framebuffer : m_frameBuffers)
		vkDestroyFramebuffer(m_deviceRef, framebuffer, nullptr);

	for (VkImageView imageView : m_swapchainImageViews)
		vkDestroyImageView(m_deviceRef, imageView, nullptr);

	vkDestroySwapchainKHR(m_deviceRef, m_swapchain, nullptr);
}

void SwapChainManager::CreateFramebuffers(
	VkDevice device, VkRenderPass renderPass, VkImageView depthImageView, std::uint32_t width,
	std::uint32_t height
) {
	m_frameBuffers.resize(std::size(m_swapchainImageViews));

	VkResult result;
	for (size_t index = 0u; index < std::size(m_swapchainImageViews); ++index) {
		VkImageView attachments[] = { m_swapchainImageViews[index], depthImageView };

		VkFramebufferCreateInfo frameBufferInfo = {};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = renderPass;
		frameBufferInfo.attachmentCount = static_cast<std::uint32_t>(std::size(attachments));
		frameBufferInfo.pAttachments = attachments;
		frameBufferInfo.width = width;
		frameBufferInfo.height = height;
		frameBufferInfo.layers = 1u;

		VK_THROW_FAILED(result,
			vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &m_frameBuffers[index])
		);
	}
}
