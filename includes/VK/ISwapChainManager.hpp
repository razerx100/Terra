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
	virtual std::uint32_t GetAvailableImageIndex() const noexcept = 0;
	virtual VkImage GetImage(std::uint32_t imageIndex) const noexcept = 0;

	virtual void PresentImage(std::uint32_t imageIndex) = 0;
	virtual void ResizeSwapchain(
		std::uint32_t width, std::uint32_t height, bool& formatChanged
	) = 0;

	virtual void GetUndefinedToTransferBarrier(
		std::uint32_t imageIndex,
		VkImageMemoryBarrier& transferBarrier
	) const noexcept = 0;
	virtual void GetTransferToPresentBarrier(
		std::uint32_t imageIndex,
		VkImageMemoryBarrier& presentBarrier
	) const noexcept = 0;
};

ISwapChainManager* CreateSwapchainManagerInstance(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	VkQueue presentQueue, std::uint32_t queueFamily
);

#endif
