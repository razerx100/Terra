#ifndef __DEVICE_MANAGER_HPP__
#define __DEVICE_MANAGER_HPP__
#include <vulkan/vulkan.h>
#include <vector>

class DeviceManager {
public:
	~DeviceManager() noexcept;

	void CreatePhysicalDevice(VkInstance instance);
	void CreateLogicalDevice();

	VkQueue GetQueue(VkQueueFlagBits type) noexcept;

	VkPhysicalDevice GetPhysicalDevice() const noexcept;

private:
	struct QueueFamilyInfo {
		std::uint32_t index = 0u;
		std::uint32_t queueRequired = 0u;
		std::uint32_t queueCreated = 0u;
		std::uint32_t typeFlags = 0u;
	};

private:
	bool CheckDeviceType(
		VkPhysicalDevice device,
		VkPhysicalDeviceType deviceType
	) const noexcept;
	bool CheckQueueFamilySupport(
		VkPhysicalDevice device
	) noexcept;

	std::uint32_t GetIndexOfQueueFamily(VkQueueFlagBits queueType) const noexcept;

	void SetSuitablePhysicalDevice(VkInstance instance);

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;
	const std::vector<const char*> m_extensionNames = {
		"VK_KHR_swapchain"
	};
};

DeviceManager* GetDeviceManagerInstance() noexcept;
void InitDeviceManagerInstance();
void CleanUpDeviceManagerInstance() noexcept;
#endif
