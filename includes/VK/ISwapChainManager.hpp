#ifndef __I_SWAPCHAIN_MANAGER_HPP__
#define __I_SWAPCHAIN_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <IDeviceManager.hpp>
#include <cstdint>

class ISwapChainManager {
public:
	virtual ~ISwapChainManager() = default;

	virtual VkSwapchainKHR GetRef() const noexcept = 0;
};

ISwapChainManager* GetSwapchainManagerInstance() noexcept;
void InitSwapchainManagerInstance(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height
);
void CleanUpSwapchainManagerInstance() noexcept;
#endif
