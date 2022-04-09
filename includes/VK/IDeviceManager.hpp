#ifndef __I_DEVICE_MANAGER_HPP__
#define __I_DEVICE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>

struct SwapChainInfo {
	VkSurfaceCapabilitiesKHR capabilities;
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

class IDeviceManager {
public:
	virtual ~IDeviceManager() = default;

	virtual void CreatePhysicalDevice(
		VkInstance instance,
		VkSurfaceKHR surface
	) = 0;
	virtual void CreateLogicalDevice() = 0;

	virtual QueueData GetQueue(QueueType type) noexcept = 0;

	[[nodiscard]]
	virtual VkPhysicalDevice GetPhysicalDevice() const noexcept = 0;
	[[nodiscard]]
	virtual VkDevice GetLogicalDevice() const noexcept = 0;
	[[nodiscard]]
	virtual SwapChainInfo GetSwapChainInfo() const noexcept = 0;
};

IDeviceManager* CreateDeviceManagerInstance();
#endif
