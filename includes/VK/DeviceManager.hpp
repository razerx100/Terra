#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

struct SwapChainInfo {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	[[nodiscard]]
	bool IsCapable() const noexcept {
		return !formats.empty() && !presentModes.empty();
	}
};

struct QueueData {
	VkQueue queueHandle;
	size_t queueFamilyIndex;
};

enum QueueType {
	TransferQueue = 1,
	ComputeQueue = 2,
	GraphicsQueue = 4,
	PresentQueue = 8
};

class DeviceManager {
public:
	~DeviceManager() noexcept;

	void CreatePhysicalDevice(
		VkInstance instance,
		VkSurfaceKHR surface
	);
	void CreateLogicalDevice();

	QueueData GetQueue(QueueType type) noexcept;

	[[nodiscard]]
	VkPhysicalDevice GetPhysicalDevice() const noexcept;
	[[nodiscard]]
	VkDevice GetLogicalDevice() const noexcept;
	[[nodiscard]]
	SwapChainInfo GetSwapChainInfo() const noexcept;

private:
	struct QueueFamilyInfo {
		size_t index = 0u;
		std::uint32_t typeFlags = 0u;
		size_t queueRequired = 0u;
		size_t queueCreated = 0u;
	};

private:
	using FamilyInfo = std::vector<std::pair<size_t, QueueType>>;

	bool CheckDeviceType(
		VkPhysicalDevice device,
		VkPhysicalDeviceType deviceType
	) const noexcept;
	bool CheckPresentSupport(
		VkPhysicalDevice device,
		VkSurfaceKHR surface,
		size_t index
	) const noexcept;
	bool CheckDeviceExtensionSupport(
		VkPhysicalDevice device
	) const noexcept;
	bool IsDeviceSuitable(
		VkPhysicalDevice device, VkSurfaceKHR surface,
		FamilyInfo& familyInfos
	) const noexcept;

	void GetQueueSupportInfo(
		VkPhysicalDevice device,
		VkSurfaceKHR surface,
		FamilyInfo& familyInfos
	) const noexcept;
	void GetSwapchainCapabilities(
		VkPhysicalDevice device,
		VkSurfaceKHR surface,
		SwapChainInfo& details
	) const noexcept;

	void SetQueueFamilyInfo(
		FamilyInfo& familyInfos
	) noexcept;

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;
	const std::vector<const char*> m_extensionNames = {
		"VK_KHR_swapchain"
	};
};
#endif
