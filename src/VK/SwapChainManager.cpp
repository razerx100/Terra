#include <SwapChainManager.hpp>
#include <VkResourceViews.hpp>

SwapChainManager::SwapChainManager(const Args& arguments)
	: m_swapchain{ VK_NULL_HANDLE }, m_deviceRef{ arguments.device.value() },
	m_swapchainFormat{}, m_swapchainExtent{}, m_presentQueue{ arguments.presentQueue.value() },
	m_surfaceInfo{ arguments.surfaceInfo.value() }, m_nextImageIndex{ 0u } {

	SwapChainManagerCreateInfo swapCreateInfo{
		.device = arguments.device.value(),
		.surface = arguments.surface.value(),
		.surfaceInfo = arguments.surfaceInfo.value(),
		.width = arguments.width.value(),
		.height = arguments.height.value(),
		.bufferCount = arguments.bufferCount.value()
	};

	auto surfaceFormat = ChooseSurfaceFormat(m_surfaceInfo.formats);
	CreateSwapchain(swapCreateInfo, surfaceFormat);
	QueryImages();
	CreateImageViews(m_deviceRef);
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
	VkRenderPass renderPass, VkImageView depthImageView,
	const VkSurfaceFormatKHR& surfaceFormat
) {
	CleanUpSwapchain();

	SwapChainManagerCreateInfo createInfo{
		.device = device,
		.surface = surface,
		.surfaceInfo = m_surfaceInfo,
		.width = width,
		.height = height,
		.bufferCount = static_cast<std::uint32_t>(std::size(m_swapchainImages))
	};

	CreateSwapchain(createInfo, surfaceFormat);
	QueryImages();
	CreateImageViews(device);
	CreateFramebuffers(device, renderPass, depthImageView, width, height);
}

VkSurfaceFormatKHR SwapChainManager::GetSurfaceFormat() const noexcept {
	return ChooseSurfaceFormat(m_surfaceInfo.formats);
}

bool SwapChainManager::HasSurfaceFormatChanged(
	const VkSurfaceFormatKHR& surfaceFormat
) const noexcept {
	return surfaceFormat.format != m_swapchainFormat ? true : false;
}

void SwapChainManager::CreateSwapchain(
	const SwapChainManagerCreateInfo& swapCreateInfo, const VkSurfaceFormatKHR& surfaceFormat
) {
	VkExtent2D swapExtent = { swapCreateInfo.width, swapCreateInfo.height };
	m_surfaceInfo.capabilities.currentExtent = swapExtent;

	m_swapchainExtent = swapExtent;
	m_swapchainFormat = surfaceFormat.format;

	VkPresentModeKHR swapPresentMode = ChoosePresentMode(
		swapCreateInfo.surfaceInfo.presentModes
	);

	std::uint32_t imageCount = swapCreateInfo.bufferCount;
	if (swapCreateInfo.surfaceInfo.capabilities.maxImageCount > 0u
		&& swapCreateInfo.surfaceInfo.capabilities.maxImageCount < imageCount)
		imageCount = swapCreateInfo.surfaceInfo.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = swapCreateInfo.surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = swapExtent,
		.imageArrayLayers = 1u,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = swapCreateInfo.surfaceInfo.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = swapPresentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	vkCreateSwapchainKHR(swapCreateInfo.device, &createInfo, nullptr, &m_swapchain);
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

	for (size_t index = 0u; index < std::size(m_swapchainImageViews); ++index) {
		VkImageView attachments[] = { m_swapchainImageViews[index], depthImageView };

		VkFramebufferCreateInfo frameBufferInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = static_cast<std::uint32_t>(std::size(attachments)),
			.pAttachments = attachments,
			.width = width,
			.height = height,
			.layers = 1u
		};

		vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &m_frameBuffers[index]);
	}
}
