#ifndef SWAPCHAIN_MANAGER_HPP_
#define SWAPCHAIN_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <DeviceManager.hpp>
#include <memory>
#include <VkHelperFunctions.hpp>
#include <optional>

class SwapChainManager {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<VkSurfaceKHR> surface;
		std::optional<SurfaceInfo> surfaceInfo;
		std::optional<std::uint32_t> width;
		std::optional<std::uint32_t> height;
		std::optional<std::uint32_t> bufferCount;
		std::optional<VkQueue> presentQueue;
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
	VkFramebuffer GetFramebuffer(size_t imageIndex) const noexcept;

	void AcquireNextImageIndex(VkSemaphore signalSemaphore) noexcept;
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
	std::vector<VkImageView> m_swapchainImageViews;
	std::vector<VkFramebuffer> m_frameBuffers;
	VkQueue m_presentQueue;
	SurfaceInfo m_surfaceInfo;
	size_t m_nextImageIndex;
};
#endif
