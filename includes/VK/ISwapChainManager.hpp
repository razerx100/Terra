#ifndef __I_SWAPCHAIN_MANAGER_HPP__
#define __I_SWAPCHAIN_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <IDeviceManager.hpp>
#include <cstdint>

class ISwapChainManager {
public:
	virtual ~ISwapChainManager() = default;

	virtual VkSwapchainKHR GetRef() const noexcept = 0;
	virtual VkExtent2D GetSwapExtent() const noexcept = 0;
	virtual VkFormat GetSwapFormat() const noexcept = 0;
	virtual size_t GetAvailableImageIndex() const noexcept = 0;
	virtual VkFramebuffer GetFramebuffer(size_t imageIndex) const noexcept = 0;
	virtual VkSemaphore GetImageSemaphore() const noexcept = 0;

	virtual void SetNextFrameIndex(size_t index) noexcept = 0;
	virtual void PresentImage(
		std::uint32_t imageIndex,
		VkSemaphore renderSemaphore
	) = 0;
	virtual bool ResizeSwapchain(
		std::uint32_t width, std::uint32_t height,
		VkRenderPass renderPass, bool& formatChanged
	) = 0;
	virtual void CreateFramebuffers(
		VkDevice device, VkRenderPass renderPass,
		std::uint32_t width, std::uint32_t height
	) = 0;
};

ISwapChainManager* CreateSwapchainManagerInstance(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, size_t bufferCount,
	VkQueue presentQueue, size_t queueFamily
);

#endif
