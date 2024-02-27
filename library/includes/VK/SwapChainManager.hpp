#ifndef SWAPCHAIN_MANAGER_HPP_
#define SWAPCHAIN_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <span>
#include <VkHelperFunctions.hpp>
#include <VkExtensionManager.hpp>
#include <VkTextureView.hpp>

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

class SwapChainManager
{
public:
	struct Args
	{
		VkDevice      device;
		VkSurfaceKHR  surface;
		SurfaceInfo   surfaceInfo;
		std::uint32_t width;
		std::uint32_t height;
		std::uint32_t bufferCount;
		VkQueue       presentQueue;
	};

public:
	SwapChainManager(const Args& arguments);
	~SwapChainManager() noexcept;

	[[nodiscard]]
	VkSwapchainKHR GetRef() const noexcept;
	[[nodiscard]]
	VkExtent2D GetSwapExtent() const noexcept;
	[[nodiscard]]
	VkFormat GetSwapFormat() const noexcept;
	[[nodiscard]]
	size_t GetNextImageIndex() const noexcept;
	[[nodiscard]]
	VkFramebuffer GetFramebuffer(size_t imageIndex) const noexcept
	{
		return m_frameBuffers.at(imageIndex).Get();
	}
	[[nodiscard]]
	bool HasSurfaceFormatChanged(const VkSurfaceFormatKHR& surfaceFormat) const noexcept;
	[[nodiscard]]
	VkSurfaceFormatKHR GetSurfaceFormat() const noexcept;

	void AcquireNextImageIndex(VkSemaphore signalSemaphore) noexcept;
	void PresentImage(std::uint32_t imageIndex) const noexcept;
	void ResizeSwapchain(
		VkDevice device, VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height,
		VkRenderPass renderPass, VkImageView depthImageView,
		const VkSurfaceFormatKHR& surfaceFormat
	);
	void CreateFramebuffers(
		VkDevice device, VkRenderPass renderPass, VkImageView depthImageView,
		std::uint32_t width, std::uint32_t height
	);

private:
	struct SwapChainManagerCreateInfo {
		VkDevice device;
		VkSurfaceKHR surface;
		SurfaceInfo surfaceInfo;
		std::uint32_t width;
		std::uint32_t height;
		std::uint32_t bufferCount;
	};

private:
	[[nodiscard]]
	VkSurfaceFormatKHR ChooseSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats
	) const noexcept;
	[[nodiscard]]
	VkPresentModeKHR ChoosePresentMode(
		const std::vector<VkPresentModeKHR>& availableModes
	) const noexcept;

	void QueryImages();
	void CreateImageViews(VkDevice device);
	void CreateSwapchain(
		const SwapChainManagerCreateInfo& swapCreateInfo,
		const VkSurfaceFormatKHR& surfaceFormat
	);
	void CleanUpSwapchain() noexcept;

private:
	VkSwapchainKHR m_swapchain;
	VkDevice m_deviceRef;
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VKImageView>   m_swapchainImageViews;
	std::vector<VKFramebuffer> m_frameBuffers;
	VkQueue m_presentQueue;
	SurfaceInfo m_surfaceInfo;
	size_t m_nextImageIndex;

	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkKhrSwapchain
	};

public:
	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{ return s_requiredExtensions; }
};
#endif
