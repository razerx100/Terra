#ifndef __I_DEVICE_MANAGER_HPP__
#define __I_DEVICE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>

class IDeviceManager {
public:
	virtual ~IDeviceManager() = default;

	virtual void CreatePhysicalDevice(
		VkInstance instance,
		VkSurfaceKHR surface
	) = 0;
	virtual void CreateLogicalDevice() = 0;

	virtual VkQueue GetQueue(VkQueueFlagBits type) noexcept = 0;

	virtual VkPhysicalDevice GetPhysicalDevice() const noexcept = 0;
};

IDeviceManager* GetDeviceManagerInstance() noexcept;
void InitDeviceManagerInstance();
void CleanUpDeviceManagerInstance() noexcept;
#endif
