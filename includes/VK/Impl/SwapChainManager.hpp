#ifndef __SWAPCHAIN_MANAGER_HPP__
#define __SWAPCHAIN_MANAGER_HPP__
#include <ISwapChainManager.hpp>

class SwapChainManager : public ISwapChainManager {
public:
	SwapChainManager(
		VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
		std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		VkQueue presentQueue, std::uint32_t queueFamily
	);
	~SwapChainManager() noexcept;

	VkSwapchainKHR GetRef() const noexcept override;
	VkExtent2D GetSwapExtent() const noexcept override;
	VkFormat GetSwapFormat() const noexcept override;
	std::uint32_t GetAvailableImageIndex() const noexcept override;
	VkImage GetImage(std::uint32_t imageIndex) const noexcept override;


	void PresentImage(std::uint32_t imageIndex) override;

	VkImageMemoryBarrier GetPresentToRenderBarrier(
		std::uint32_t imageIndex
	) const noexcept override;
	VkImageMemoryBarrier GetRenderToPresentBarrier(
		std::uint32_t imageIndex
	) const noexcept override;

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

	void CreateImageViews(VkDevice device);

private:
	VkSwapchainKHR m_swapchain;
	VkDevice m_deviceRef;
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkQueue m_presentQueue;
	std::uint32_t m_presentFamilyIndex;
};
#endif
