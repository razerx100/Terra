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
	~SwapChainManager() noexcept override;

	[[nodiscard]]
	VkSwapchainKHR GetRef() const noexcept override;
	[[nodiscard]]
	VkExtent2D GetSwapExtent() const noexcept override;
	[[nodiscard]]
	VkFormat GetSwapFormat() const noexcept override;
	[[nodiscard]]
	size_t GetAvailableImageIndex() const noexcept override;
	[[nodiscard]]
	VkFramebuffer GetFramebuffer(size_t imageIndex) const noexcept override;
	[[nodiscard]]
	VkSemaphore GetImageSemaphore() const noexcept override;

	void SetNextFrameIndex(size_t index) noexcept override;
	void PresentImage(
		std::uint32_t imageIndex,
		VkSemaphore renderSemaphore
	) override;
	bool ResizeSwapchain(
		std::uint32_t width, std::uint32_t height,
		VkRenderPass renderPass, bool& formatChanged
	) override;
	void CreateFramebuffers(
		VkDevice device, VkRenderPass renderPass,
		std::uint32_t width, std::uint32_t height
	) override;

private:
	[[nodiscard]]
	VkSurfaceFormatKHR ChooseSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats
	) const noexcept;
	[[nodiscard]]
	VkPresentModeKHR ChoosePresentMode(
		const std::vector<VkPresentModeKHR>& availableModes
	) const noexcept;
	[[nodiscard]]
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
