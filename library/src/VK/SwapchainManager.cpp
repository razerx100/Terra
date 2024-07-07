#include <SwapchainManager.hpp>
#include <array>

// VkSwapchain
VkSwapchain::VkSwapchain(VkDevice device, std::uint32_t bufferCount)
	: m_device{ device }, m_swapchain{ VK_NULL_HANDLE }, m_swapImageViewFormat{ VK_FORMAT_UNDEFINED },
	m_swapchainExtent{ 0u, 0u },
	m_swapchainImages{ bufferCount, VK_NULL_HANDLE }, m_swapchainImageViews{},
	m_frameBuffers{}
{
	m_swapchainImageViews.reserve(static_cast<size_t>(bufferCount));
	m_frameBuffers.reserve(static_cast<size_t>(bufferCount));
}

void VkSwapchain::Create(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice, const SurfaceManager& surface
) {
	// Create the swapchain.
	const VkPresentModeKHR swapPresentMode = surface.GetPresentMode(physicalDevice);
	const VkSurfaceFormatKHR surfaceFormat = surface.GetSurfaceFormat(physicalDevice);
	const VkSurfaceCapabilitiesKHR surfaceCapabilities = surface.GetSurfaceCapabilities(
		physicalDevice
	);

	{
		const std::uint32_t imageCount = std::max(
			surfaceCapabilities.maxImageCount, static_cast<std::uint32_t>(std::size(m_swapchainImages))
		);
		const VkExtent2D swapExtent = surfaceCapabilities.currentExtent;

		VkSwapchainCreateInfoKHR createInfo{
			.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface          = surface.Get(),
			.minImageCount    = imageCount,
			.imageFormat      = surfaceFormat.format,
			.imageColorSpace  = surfaceFormat.colorSpace,
			.imageExtent      = swapExtent,
			.imageArrayLayers = 1u,
			.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform     = surfaceCapabilities.currentTransform,
			.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode      = swapPresentMode,
			.clipped          = VK_TRUE,
			.oldSwapchain     = VK_NULL_HANDLE
		};

		if (m_swapchain != VK_NULL_HANDLE)
			SelfDestruct();

		vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &m_swapchain);

		m_swapchainExtent = swapExtent;
	}

	// Query the VkImages.
	{
		std::uint32_t imageCount = 0u;
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);

		m_swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, std::data(m_swapchainImages));
	}

	// Make the ImageViews.
	{
		const size_t swapImageCount     = std::size(m_swapchainImages);
		const size_t swapImageViewCount = std::size(m_swapchainImageViews);

		// Only add new image views if there are new images.
		for (size_t index = swapImageViewCount; index < swapImageCount; ++index)
			m_swapchainImageViews.emplace_back(logicalDevice);

		for (size_t index = 0u; index < std::size(m_swapchainImageViews); ++index)
			m_swapchainImageViews.at(index).CreateView(
				m_swapchainImages.at(index), surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_VIEW_TYPE_2D
			);

		m_swapImageViewFormat = surfaceFormat.format;
	}
}

void VkSwapchain::CreateFramebuffers(
	VkDevice device, const VKRenderPass& renderPass, const DepthBuffer& depthBuffer,
	std::uint32_t width, std::uint32_t height
) {
	const size_t swapImageViewCount = std::size(m_swapchainImageViews);
	const size_t frameBufferCount   = std::size(m_frameBuffers);

	// Only add new frame buffers if there are new imageviews.
	for (size_t index = frameBufferCount; index < swapImageViewCount; ++index)
		m_frameBuffers.emplace_back(device);

	for (size_t index = 0u; index < std::size(m_frameBuffers); ++index)
	{
		VkImageView attachments[] = { m_swapchainImageViews.at(index).Get(), depthBuffer.GetView() };

		m_frameBuffers.at(index).Create(renderPass, width, height, attachments);
	}
}

VkSwapchain::~VkSwapchain() noexcept
{
	SelfDestruct();
}

void VkSwapchain::SelfDestruct() noexcept
{
	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

// Swapchain Manager
SwapchainManager::SwapchainManager(VkDevice device, VkQueue presentQueue, std::uint32_t bufferCount)
	: m_presentQueue{ presentQueue }, m_swapchain{ device, bufferCount }, m_nextImageIndex{ 0u },
	m_hasSwapchainFormatChanged{ false }
{}

void SwapchainManager::Present(
	std::uint32_t imageIndex, const VKSemaphore& waitSemaphore
) const noexcept {
	const VkSemaphore semapohores[]   = { waitSemaphore.Get() };
	const VkSwapchainKHR swapchains[] = { m_swapchain.Get() };

	VkPresentInfoKHR presentInfo{
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores    = semapohores,
		.swapchainCount     = static_cast<std::uint32_t>(std::size(swapchains)),
		.pSwapchains        = swapchains,
		.pImageIndices      = &imageIndex,
		.pResults           = nullptr
	};

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
}

void SwapchainManager::CreateSwapchain(
	const VkDeviceManager& deviceManager, const SurfaceManager& surface
) {
	const VkFormat oldFormat        = m_swapchain.GetFormat();
	VkDevice logicalDevice          = deviceManager.GetLogicalDevice();
	VkPhysicalDevice physicalDevice = deviceManager.GetPhysicalDevice();

	m_swapchain.Create(logicalDevice, physicalDevice, surface);

	const VkFormat newFormat = m_swapchain.GetFormat();

	m_hasSwapchainFormatChanged = oldFormat != newFormat;
}

void SwapchainManager::CreateFramebuffers(
	VkDevice device, const VKRenderPass& renderPass, const DepthBuffer& depthBuffer
) {
	const VkExtent2D swapchainExtent = GetCurrentSwapchainExtent();

	m_swapchain.CreateFramebuffers(
		device, renderPass, depthBuffer, swapchainExtent.width, swapchainExtent.height
	);
}

void SwapchainManager::QueryNextImageIndex(VkDevice device, VkSemaphore waitForImageSemaphore)
{
	vkAcquireNextImageKHR(
		device, m_swapchain.Get(), UINT64_MAX, waitForImageSemaphore, VK_NULL_HANDLE,
		&m_nextImageIndex
	);
}
