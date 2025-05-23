#ifndef VK_SWAPCHAIN_MANAGER_HPP_
#define VK_SWAPCHAIN_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <span>
#include <VkExtensionManager.hpp>
#include <VkTextureView.hpp>
#include <VkSurfaceManager.hpp>
#include <VkFramebuffer.hpp>
#include <VkSyncObjects.hpp>
#include <VkDeviceManager.hpp>

namespace Terra
{
class VkSwapchain
{
public:
	VkSwapchain(VkDevice device, std::uint32_t bufferCount);
	~VkSwapchain() noexcept;

	void Create(
		VkPhysicalDevice physicalDevice, const SurfaceManager& surface,
		std::uint32_t width, std::uint32_t height
	);

	[[nodiscard]]
	size_t GetImageCount() const noexcept { return std::size(m_swapchainImages); }
	[[nodiscard]]
	VkSwapchainKHR Get() const noexcept { return m_swapchain; }
	[[nodiscard]]
	VkFormat GetFormat() const noexcept { return m_swapImageViewFormat; }
	[[nodiscard]]
	VkExtent2D GetExtent() const noexcept { return m_swapchainExtent; }
	[[nodiscard]]
	const VKImageView& GetColourAttachment(size_t index) const noexcept
	{
		return m_swapchainImageViews[index];
	}

private:
	void SelfDestruct() noexcept;

private:
	VkDevice                 m_device;
	VkSwapchainKHR           m_swapchain;
	VkFormat                 m_swapImageViewFormat;
	VkExtent2D               m_swapchainExtent;
	std::vector<VkImage>     m_swapchainImages;
	std::vector<VKImageView> m_swapchainImageViews;

public:
	VkSwapchain(const VkSwapchain&) = delete;
	VkSwapchain& operator=(const VkSwapchain&) = delete;

	VkSwapchain(VkSwapchain&& other) noexcept
		: m_device{ other.m_device }, m_swapchain{ std::exchange(other.m_swapchain, VK_NULL_HANDLE) },
		m_swapImageViewFormat{ other.m_swapImageViewFormat },
		m_swapchainExtent{ other.m_swapchainExtent },
		m_swapchainImages{ std::move(other.m_swapchainImages) },
		m_swapchainImageViews{ std::move(other.m_swapchainImageViews) }
	{}
	VkSwapchain& operator=(VkSwapchain&& other) noexcept
	{
		SelfDestruct();

		m_device              = other.m_device;
		m_swapchain           = std::exchange(other.m_swapchain, VK_NULL_HANDLE);
		m_swapImageViewFormat = other.m_swapImageViewFormat;
		m_swapchainExtent     = other.m_swapchainExtent;
		m_swapchainImages     = std::move(other.m_swapchainImages);
		m_swapchainImageViews = std::move(other.m_swapchainImageViews);

		return *this;
	}
};

class SwapchainManager
{
public:
	SwapchainManager(VkDevice device, VkQueue presentQueue, std::uint32_t bufferCount);

	void Present(std::uint32_t imageIndex, VkSemaphore waitSemaphore) const;
	void Present(std::uint32_t imageIndex, const VKSemaphore& waitSemaphore) const
	{
		Present(imageIndex, waitSemaphore.Get());
	}

	void CreateSwapchain(
		VkPhysicalDevice physicalDevice, const SurfaceManager& surface,
		std::uint32_t width, std::uint32_t height
	);
	void QueryNextImageIndex(VkDevice device);

	[[nodiscard]]
	VkExtent2D GetCurrentSwapchainExtent() const noexcept { return m_swapchain.GetExtent(); }
	[[nodiscard]]
	VkFormat GetSwapchainFormat() const noexcept { return m_swapchain.GetFormat(); }
	[[nodiscard]]
	bool HasSwapchainFormatChanged() const noexcept { return m_hasSwapchainFormatChanged; }

	[[nodiscard]]
	std::uint32_t GetNextImageIndex() const noexcept { return m_nextImageIndex; };
	[[nodiscard]]
	const VKSemaphore& GetNextImageSemaphore() const noexcept { return m_imageSemaphore; }
	[[nodiscard]]
	VkSwapchainKHR GetVkSwapchain() const noexcept { return m_swapchain.Get(); }
	[[nodiscard]]
	const VkSwapchain& GetSwapchain() const noexcept { return m_swapchain; }
	[[nodiscard]]
	const VKImageView& GetColourAttachment(size_t index) const noexcept
	{
		return m_swapchain.GetColourAttachment(index);
	}

private:
	VkQueue       m_presentQueue;
	VkSwapchain   m_swapchain;
	// It should be okay to have a single sync object,
	// since we would need to wait on the CPU for some fences anyway.
	VKSemaphore   m_imageSemaphore;
	std::uint32_t m_nextImageIndex;
	bool          m_hasSwapchainFormatChanged;

	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkKhrSwapchain
	};

public:
	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{ return s_requiredExtensions; }

	SwapchainManager(const SwapchainManager&) = delete;
	SwapchainManager& operator=(const SwapchainManager&) = delete;

	SwapchainManager(SwapchainManager&& other) noexcept
		: m_presentQueue{ other.m_presentQueue },
		m_swapchain{ std::move(other.m_swapchain) },
		m_imageSemaphore{ std::move(other.m_imageSemaphore) },
		m_nextImageIndex{ other.m_nextImageIndex },
		m_hasSwapchainFormatChanged{ other.m_hasSwapchainFormatChanged }
	{}
	SwapchainManager& operator=(SwapchainManager&& other) noexcept
	{
		m_presentQueue              = other.m_presentQueue;
		m_swapchain                 = std::move(other.m_swapchain);
		m_imageSemaphore            = std::move(other.m_imageSemaphore);
		m_nextImageIndex            = other.m_nextImageIndex;
		m_hasSwapchainFormatChanged = other.m_hasSwapchainFormatChanged;

		return *this;
	}
};
}
#endif
