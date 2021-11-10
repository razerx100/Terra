#ifndef __DEVICE_MANAGER_HPP__
#define __DEVICE_MANAGER_HPP__
#include <vulkan/vulkan.h>
#include <vector>

class DeviceManager {
public:
	void CreatePhysicalDevice(VkInstance instance);

	void AddQueueTypeCheck(VkQueueFlagBits queueType) noexcept;
	VkPhysicalDevice GetPhysicalDevice() const noexcept;

private:
	void SetSuitablePhysicalDevice(VkInstance instance);
	bool CheckDeviceType(
		VkPhysicalDevice device,
		VkPhysicalDeviceType deviceType
	) const noexcept;
	bool CheckQueueFamilySupport(
		VkPhysicalDevice device
	) const noexcept;

private:
	VkPhysicalDevice m_physicalDevice;
	std::vector<VkQueueFlagBits> m_queueTypeFlag;
};

DeviceManager* GetDeviceManagerInstance() noexcept;
void InitDeviceManagerInstance();
void CleanUpDeviceManagerInstance() noexcept;
#endif
