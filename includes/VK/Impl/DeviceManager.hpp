#ifndef __DEVICE_MANAGER_HPP__
#define __DEVICE_MANAGER_HPP__
#include <IDeviceManager.hpp>
#include <vector>

class DeviceManager : public IDeviceManager {
public:
	~DeviceManager() noexcept;

	void CreatePhysicalDevice(
		VkInstance instance,
		VkSurfaceKHR surface
	) override;
	void CreateLogicalDevice() override;

	VkQueue GetQueue(VkQueueFlagBits type) noexcept override;

	VkPhysicalDevice GetPhysicalDevice() const noexcept override;

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
		VkPhysicalDevice device,
		VkSurfaceKHR surface
	) noexcept;
	bool CheckPresentSupport(
		VkPhysicalDevice device,
		VkSurfaceKHR surface,
		std::uint32_t index
	) const noexcept;

	std::uint32_t GetIndexOfQueueFamily(VkQueueFlagBits queueType) const noexcept;

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;
	const std::vector<const char*> m_extensionNames = {
		"VK_KHR_swapchain"
	};
};
#endif
