#ifndef __SWAPCHAIN_MANAGER_HPP__
#define __SWAPCHAIN_MANAGER_HPP__
#include <ISwapChainManager.hpp>
#include <ISemaphoreWrapper.hpp>
#include <memory>

class SwapChainManager : public ISwapChainManager {
public:
	SwapChainManager(
		VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
		std::uint32_t width, std::uint32_t height, size_t bufferCount,
		VkQueue presentQueue, size_t queueFamily
	);
	~SwapChainManager() noexcept;

	VkSwapchainKHR GetRef() const noexcept override;
	VkExtent2D GetSwapExtent() const noexcept override;
	VkFormat GetSwapFormat() const noexcept override;
	size_t GetAvailableImageIndex() const noexcept override;
	VkFramebuffer GetFramebuffer(size_t imageIndex) const noexcept override;
	VkSemaphore GetImageSemaphore() const noexcept override;

	void SetNextFrameIndex(size_t index) noexcept override;
	void PresentImage(
		std::uint32_t imageIndex,
		VkSemaphore renderSemaphore
	) override;
	void ResizeSwapchain(
		std::uint32_t width, std::uint32_t height,
		VkRenderPass renderPass, bool& formatChanged
	) override;
	void CreateFramebuffers(
		VkDevice device, VkRenderPass renderPass,
		std::uint32_t width, std::uint32_t height
	) override;

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

	void QueryImages();
	void CreateImageViews();
	void CreateSwapchain(
		VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
		size_t bufferCount, std::uint32_t width, std::uint32_t height,
		bool& formatChanged
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
	size_t m_presentFamilyIndex;
	std::unique_ptr<ISemaphoreWrapper> m_imageSemaphore;
	size_t m_currentFrameIndex;
};
#endif
