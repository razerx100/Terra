#ifndef SWAPCHAIN_MANAGER_HPP_
#define SWAPCHAIN_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <DeviceManager.hpp>
#include <memory>
#include <VkHelperFunctions.hpp>

struct SwapChainManagerCreateInfo {
	VkDevice device;
	VkSurfaceKHR surface;
	SurfaceInfo surfaceInfo;
	std::uint32_t width;
	std::uint32_t height;
	std::uint32_t bufferCount;
};

class SwapChainManager {
public:
	SwapChainManager(
		const SwapChainManagerCreateInfo& swapCreateInfo, VkQueue presentQueue
	);
	~SwapChainManager() noexcept;

	[[nodiscard]]
	VkSwapchainKHR GetRef() const noexcept;
	[[nodiscard]]
	VkExtent2D GetSwapExtent() const noexcept;
	[[nodiscard]]
	VkFormat GetSwapFormat() const noexcept;
	[[nodiscard]]
	size_t GetAvailableImageIndex(VkSemaphore semaphore) const noexcept;
	[[nodiscard]]
	VkFramebuffer GetFramebuffer(size_t imageIndex) const noexcept;

	void PresentImage(std::uint32_t imageIndex) const noexcept;
	void ResizeSwapchain(
		VkDevice device, VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height,
		VkRenderPass renderPass, VkImageView depthImageView, bool& formatChanged
	);
	void CreateFramebuffers(
		VkDevice device, VkRenderPass renderPass, VkImageView depthImageView,
		std::uint32_t width, std::uint32_t height
	);

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
		const SwapChainManagerCreateInfo& swapCreateInfo, bool& formatChanged
	);
	void CleanUpSwapchain() noexcept;

private:
	VkSwapchainKHR m_swapchain;
	VkDevice m_deviceRef;
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_frameBuffers;
	VkQueue m_presentQueue;
	SurfaceInfo m_surfaceInfo;
};
#endif
