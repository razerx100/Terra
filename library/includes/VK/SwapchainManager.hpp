#ifndef SWAPCHAIN_MANAGER_HPP_
#define SWAPCHAIN_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <span>
#include <VkExtensionManager.hpp>
#include <VkTextureView.hpp>
#include <SurfaceManager.hpp>
#include <VKRenderPass.hpp>
#include <DepthBuffer.hpp>
#include <VkSyncObjects.hpp>

class VKFramebuffer
{
public:
	VKFramebuffer(VkDevice device) : m_device{ device }, m_framebuffer{ VK_NULL_HANDLE } {}
	~VKFramebuffer() noexcept;

	void Create(
		VkRenderPass renderPass, std::uint32_t width, std::uint32_t height,
		std::span<VkImageView> attachments
	);

	[[nodiscard]]
	VkFramebuffer Get() const noexcept { return m_framebuffer; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice      m_device;
	VkFramebuffer m_framebuffer;

public:
	VKFramebuffer(const VKFramebuffer&) = delete;
	VKFramebuffer& operator=(const VKFramebuffer&) = delete;

	VKFramebuffer(VKFramebuffer&& other) noexcept
		: m_device{ other.m_device }, m_framebuffer{ other.m_framebuffer }
	{
		other.m_framebuffer = VK_NULL_HANDLE;
	}
	VKFramebuffer& operator=(VKFramebuffer&& other) noexcept
	{
		SelfDestruct();

		m_device            = other.m_device;
		m_framebuffer       = other.m_framebuffer;
		other.m_framebuffer = VK_NULL_HANDLE;

		return *this;
	}
};

class VkSwapchain
{
public:
	VkSwapchain(VkDevice device, std::uint32_t bufferCount);
	~VkSwapchain() noexcept;

	VkSwapchain& Create(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice, const SurfaceManager& surface,
		const VkSurfaceCapabilitiesKHR& surfaceCapabilities
	);
	VkSwapchain& CreateFramebuffers(
		VkDevice device, VkRenderPass renderPass, VkImageView depthImageView,
		std::uint32_t width, std::uint32_t height
	);

	[[nodiscard]]
	std::uint32_t GetImageCount() const noexcept
	{
		return static_cast<std::uint32_t>(std::size(m_swapchainImages));
	}
	[[nodiscard]]
	VkSwapchainKHR Get() const noexcept { return m_swapchain; }
	[[nodiscard]]
	VkFormat GetFormat() const noexcept { return m_swapImageViewFormat; }
	[[nodiscard]]
	VkExtent2D GetExtent() const noexcept { return m_swapchainExtent; }
	[[nodiscard]]
	VkFramebuffer GetFramebuffer(size_t imageIndex) const noexcept
	{
		return m_frameBuffers.at(imageIndex).Get();
	}

private:
	void SelfDestruct() noexcept;

private:
	VkDevice                   m_device;
	VkSwapchainKHR             m_swapchain;
	VkFormat                   m_swapImageViewFormat;
	VkExtent2D                 m_swapchainExtent;
	std::vector<VkImage>       m_swapchainImages;
	std::vector<VKImageView>   m_swapchainImageViews;
	std::vector<VKFramebuffer> m_frameBuffers;

public:
	VkSwapchain(const VkSwapchain&) = delete;
	VkSwapchain& operator=(const VkSwapchain&) = delete;

	VkSwapchain(VkSwapchain&& other) noexcept
		: m_device{ other.m_device }, m_swapchain{ other.m_swapchain },
		m_swapImageViewFormat{ other.m_swapImageViewFormat },
		m_swapchainExtent{ other.m_swapchainExtent },
		m_swapchainImages{ std::move(other.m_swapchainImages) },
		m_swapchainImageViews{ std::move(other.m_swapchainImageViews) },
		m_frameBuffers{ std::move(other.m_frameBuffers) }
	{
		other.m_swapchain = VK_NULL_HANDLE;
	}
	VkSwapchain& operator=(VkSwapchain&& other) noexcept
	{
		SelfDestruct();

		m_device              = other.m_device;
		m_swapchain           = other.m_swapchain;
		m_swapImageViewFormat = other.m_swapImageViewFormat;
		m_swapchainExtent     = other.m_swapchainExtent;
		m_swapchainImages     = std::move(other.m_swapchainImages);
		m_swapchainImageViews = std::move(other.m_swapchainImageViews);
		m_frameBuffers        = std::move(other.m_frameBuffers);
		other.m_swapchain     = VK_NULL_HANDLE;

		return *this;
	}
};

class SwapchainManager
{
public:
	SwapchainManager(
		VkDevice device, VkQueue presentQueue, std::uint32_t bufferCount, MemoryManager* memoryManager
	);

	void PresentImage(std::uint32_t imageIndex) const noexcept;
	void CreateSwapchain(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice, MemoryManager* memoryManager,
		const SurfaceManager& surface
	);
	void BeginRenderPass(
		VkCommandBuffer graphicsCmdBuffer, const VkClearColorValue& clearColour, size_t frameIndex
	);
	void QueryNextImageIndex(VkDevice device, VkSemaphore waitForImageSemaphore);

	[[nodiscard]]
	std::uint32_t GetNextImageIndex() const noexcept { return m_nextImageIndex; };
	[[nodiscard]]
	VkSwapchainKHR GetSwapchain() const noexcept { return m_swapchain.Get(); }
	[[nodiscard]]
	const VKSemaphore& GetSwapchainWaitSemaphore(size_t index) const noexcept
	{
		return m_waitSemaphores.at(index);
	}

private:
	VkQueue                  m_presentQueue;
	VKRenderPass             m_renderPass;
	DepthBuffer              m_depthBuffer;
	VkSwapchain              m_swapchain;
	std::vector<VKSemaphore> m_waitSemaphores;
	std::uint32_t            m_nextImageIndex;

	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkKhrSwapchain
	};

public:
	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{ return s_requiredExtensions; }

	SwapchainManager(const SwapchainManager&) = delete;
	SwapchainManager& operator=(const SwapchainManager&) = delete;

	SwapchainManager(SwapchainManager&& other) noexcept
		: m_presentQueue{ other.m_presentQueue }, m_renderPass{ std::move(other.m_renderPass) },
		m_depthBuffer{ std::move(other.m_depthBuffer) }, m_swapchain{ std::move(other.m_swapchain) },
		m_waitSemaphores{ std::move(other.m_waitSemaphores) },
		m_nextImageIndex{ other.m_nextImageIndex }
	{}
	SwapchainManager& operator=(SwapchainManager&& other) noexcept
	{
		m_presentQueue   = other.m_presentQueue;
		m_renderPass     = std::move(other.m_renderPass);
		m_depthBuffer    = std::move(other.m_depthBuffer);
		m_swapchain      = std::move(other.m_swapchain);
		m_waitSemaphores = std::move(other.m_waitSemaphores);
		m_nextImageIndex = other.m_nextImageIndex;

		return *this;
	}
};
#endif
