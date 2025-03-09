#include <SwapchainManager.hpp>
#include <array>

// VkSwapchain
VkSwapchain::VkSwapchain(VkDevice device, std::uint32_t bufferCount)
	: m_device{ device }, m_swapchain{ VK_NULL_HANDLE }, m_swapImageViewFormat{ VK_FORMAT_UNDEFINED },
	m_swapchainExtent{ 0u, 0u },
	m_swapchainImages{ bufferCount, VK_NULL_HANDLE }, m_swapchainImageViews{}
{
	m_swapchainImageViews.reserve(static_cast<size_t>(bufferCount));
}

void VkSwapchain::Create(
	VkPhysicalDevice physicalDevice, const SurfaceManager& surface,
	std::uint32_t width, std::uint32_t height
) {
	// Create the swapchain.
	const VkPresentModeKHR swapPresentMode             = surface.GetPresentMode(physicalDevice);
	const VkSurfaceFormatKHR surfaceFormat             = surface.GetSurfaceFormat(physicalDevice);
	const VkSurfaceCapabilitiesKHR surfaceCapabilities = surface.GetSurfaceCapabilities(
		physicalDevice
	);

	{
		// If the max image count is less than the amount of images we want, select the maxImageCount.
		// Otherwise, choose the number of images we want.
		const std::uint32_t imageCount = std::min(
			surfaceCapabilities.maxImageCount, static_cast<std::uint32_t>(std::size(m_swapchainImages))
		);

		const VkExtent2D desiredSwapExtent{ .width = width, .height = height };

		// The desired extent and the allowed extent might be different.
		// For example, the title area of a window can't be drawn upon.
		// So, the actual rendering area would be a bit smaller. So, select the
		// smaller extent.

		const VkExtent2D swapExtent = []
			(VkExtent2D desiredSwapExtent, VkExtent2D allowedSwapExtent) -> VkExtent2D
			{
				const bool widthCheck  = desiredSwapExtent.width <= allowedSwapExtent.width;
				const bool heightCheck = desiredSwapExtent.height <= allowedSwapExtent.height;

				return widthCheck && heightCheck ? desiredSwapExtent : allowedSwapExtent;
			}(desiredSwapExtent, surfaceCapabilities.currentExtent);

		VkSwapchainCreateInfoKHR createInfo{
			.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface          = surface.Get(),
			.minImageCount    = imageCount,
			.imageFormat      = surfaceFormat.format,
			.imageColorSpace  = surfaceFormat.colorSpace,
			.imageExtent      = swapExtent,
			.imageArrayLayers = 1u,
			.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform     = surfaceCapabilities.currentTransform,
			.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode      = swapPresentMode,
			.clipped          = VK_TRUE,
			.oldSwapchain     = VK_NULL_HANDLE
		};

		if (m_swapchain != VK_NULL_HANDLE)
			SelfDestruct();

		vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);

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
			m_swapchainImageViews.emplace_back(m_device);

		for (size_t index = 0u; index < std::size(m_swapchainImageViews); ++index)
			m_swapchainImageViews[index].CreateView(
				m_swapchainImages[index], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_VIEW_TYPE_2D
			);

		m_swapImageViewFormat = surfaceFormat.format;
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
	: m_presentQueue{ presentQueue }, m_swapchain{ device, bufferCount },
	m_imageSemaphore{ device }, m_nextImageIndex{ 0u }, m_hasSwapchainFormatChanged{ false }
{
	m_imageSemaphore.Create();
}

void SwapchainManager::Present(
	std::uint32_t imageIndex, VkSemaphore waitSemaphore
) const {
	const VkSemaphore semapohores[]   = { waitSemaphore };
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
	VkPhysicalDevice physicalDevice, const SurfaceManager& surface,
	std::uint32_t width, std::uint32_t height
) {
	const VkFormat oldFormat = m_swapchain.GetFormat();

	m_swapchain.Create(physicalDevice, surface, width, height);

	const VkFormat newFormat = m_swapchain.GetFormat();

	m_hasSwapchainFormatChanged = oldFormat != newFormat;
}

void SwapchainManager::QueryNextImageIndex(VkDevice device)
{
	vkAcquireNextImageKHR(
		device, m_swapchain.Get(), UINT64_MAX, m_imageSemaphore.Get(), VK_NULL_HANDLE,
		&m_nextImageIndex
	);
}
