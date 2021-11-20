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
};

ISwapChainManager* GetSwapchainManagerInstance() noexcept;
void InitSwapchainManagerInstance(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount
);
void CleanUpSwapchainManagerInstance() noexcept;
#endif
