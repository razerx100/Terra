#include <SwapChainManager.hpp>
#include <array>

// Framebuffer
VKFramebuffer::~VKFramebuffer() noexcept
{
	SelfDestruct();
}

void VKFramebuffer::SelfDestruct() noexcept
{
	vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
}

void VKFramebuffer::Create(
	VkRenderPass renderPass, std::uint32_t width, std::uint32_t height,
	std::span<VkImageView> attachments
) {
	VkFramebufferCreateInfo frameBufferInfo
	{
		.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass      = renderPass,
		.attachmentCount = static_cast<std::uint32_t>(std::size(attachments)),
		.pAttachments    = std::data(attachments),
		.width           = width,
		.height          = height,
		.layers          = 1u
	};

	vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &m_framebuffer);
}

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

VkSwapchain& VkSwapchain::Create(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice, const SurfaceManager& surface,
	std::uint32_t width, std::uint32_t height
) {
	// Create the swapchain.
	const VkPresentModeKHR swapPresentMode             = surface.GetPresentMode(physicalDevice);
	const VkSurfaceCapabilitiesKHR surfaceCapabilities = surface.GetSurfaceCapabilities(physicalDevice);
	const VkSurfaceFormatKHR surfaceFormat             = surface.GetSurfaceFormat(physicalDevice);

	{
		const std::uint32_t imageCount = std::max(
			surfaceCapabilities.maxImageCount, static_cast<std::uint32_t>(std::size(m_swapchainImages))
		);
		const VkExtent2D swapExtent{ width, height };

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
		for (size_t index = 0u; index < std::size(m_swapchainImages); ++index)
			m_swapchainImageViews.emplace_back(logicalDevice).CreateView(
				m_swapchainImages.at(index), surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_VIEW_TYPE_2D
			);

		m_swapImageViewFormat = surfaceFormat.format;
	}

	return *this;
}

VkSwapchain& VkSwapchain::CreateFramebuffers(
	VkDevice device, VkRenderPass renderPass, VkImageView depthImageView,
	std::uint32_t width, std::uint32_t height
) {
	for (size_t index = 0u; index < std::size(m_swapchainImageViews); ++index)
	{
		VkImageView attachments[] = { m_swapchainImageViews.at(index).Get(), depthImageView };

		m_frameBuffers.emplace_back(device).Create(renderPass, width, height, attachments);
	}

	return *this;
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
SwapchainManager::SwapchainManager(
	VkDevice device, VkQueue presentQueue, std::uint32_t bufferCount, MemoryManager* memoryManager
)
	: m_presentQueue{ presentQueue }, m_renderPass{ device },
	m_depthBuffer{ device, memoryManager }, m_swapchain{ device, bufferCount },
	m_nextImageIndex{ 0u }
{}

void SwapchainManager::PresentImage(std::uint32_t imageIndex) const noexcept
{
	VkPresentInfoKHR presentInfo{
		.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pImageIndices  = &imageIndex,
		.pResults       = nullptr
	};

	const VkSwapchainKHR swapchains[] = { m_swapchain.Get() };

	presentInfo.swapchainCount = std::size(swapchains);
	presentInfo.pSwapchains    = swapchains;

	vkQueuePresentKHR(m_presentQueue, &presentInfo);
}

void SwapchainManager::CreateSwapchain(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice, MemoryManager* memoryManager,
	const SurfaceManager& surface, std::uint32_t width, std::uint32_t height
) {
	m_depthBuffer = DepthBuffer{ logicalDevice, memoryManager };
	m_depthBuffer.Create(width, height);

	const VkFormat oldFormat = m_swapchain.GetFormat();

	m_swapchain = VkSwapchain{ logicalDevice, m_swapchain.GetImageCount() };
	m_swapchain.Create(logicalDevice, physicalDevice, surface, width, height);

	const VkFormat newFormat = m_swapchain.GetFormat();

	if (oldFormat != newFormat)
	{
		m_renderPass = VKRenderPass{ logicalDevice };
		m_renderPass.Create(
			RenderPassBuilder{}
			.AddColourAttachment(newFormat)
			.AddDepthAttachment(m_depthBuffer.GetFormat())
			.Build()
		);
	}

	m_swapchain.CreateFramebuffers(
		logicalDevice, m_renderPass.Get(), m_depthBuffer.GetView(), width, height
	);
}

void SwapchainManager::BeginRenderPass(
	VkCommandBuffer graphicsCmdBuffer, const VkClearColorValue& clearColour, size_t frameIndex
) {
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color        = clearColour;
	clearValues[1].depthStencil = m_depthBuffer.GetClearValues();

	m_renderPass.BeginPass(
		graphicsCmdBuffer, m_swapchain.GetFramebuffer(frameIndex), m_swapchain.GetExtent(),
		clearValues
	);
}

void SwapchainManager::QueryNextImageIndex(VkDevice device, VkSemaphore waitForImageSemaphore)
{
	vkAcquireNextImageKHR(
		device, m_swapchain.Get(), UINT64_MAX, waitForImageSemaphore, VK_NULL_HANDLE,
		&m_nextImageIndex
	);
}
