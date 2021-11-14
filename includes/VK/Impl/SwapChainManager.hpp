#ifndef __SWAPCHAIN_MANAGER_HPP__
#define __SWAPCHAIN_MANAGER_HPP__
#include <ISwapChainManager.hpp>

class SwapChainManager : public ISwapChainManager {
public:
	SwapChainManager(
		VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
		std::uint32_t width, std::uint32_t height
	);

	VkSwapchainKHR GetRef() const noexcept override;

private:
	VkSurfaceFormatKHR ChooseSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats
	) const noexcept;
	VkPresentModeKHR ChoosePresentMode(
		const std::vector<VkPresentModeKHR>& availableModes
	) const noexcept;
	VkExtent2D ChooseSwapExtent(
		const VkSurfaceCapabilitiesKHR& capabilities,
		std::uint32_t width, std::uint32_t height
	) const noexcept;

private:
	VkSwapchainKHR m_swapchain;
};
#endif
