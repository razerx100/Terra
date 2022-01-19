#ifndef __DEVICE_MANAGER_HPP__
#define __DEVICE_MANAGER_HPP__
#include <IDeviceManager.hpp>

class DeviceManager : public IDeviceManager {
public:
	~DeviceManager() noexcept;

	void CreatePhysicalDevice(
		VkInstance instance,
		VkSurfaceKHR surface
	) override;
	void CreateLogicalDevice() override;

	QueueData GetQueue(QueueType type) noexcept override;

	VkPhysicalDevice GetPhysicalDevice() const noexcept override;
	VkDevice GetLogicalDevice() const noexcept override;
	SwapChainInfo GetSwapChainInfo() const noexcept override;

private:
	struct QueueFamilyInfo {
		size_t index = 0u;
		std::uint32_t typeFlags = 0u;
		size_t queueRequired = 0u;
		size_t queueCreated = 0u;
	};

private:
	bool CheckDeviceType(
		VkPhysicalDevice device,
		VkPhysicalDeviceType deviceType
	) const noexcept;
	bool CheckPresentSupport(
		VkPhysicalDevice device,
		VkSurfaceKHR surface,
		std::uint32_t index
	) const noexcept;
	bool CheckDeviceExtensionSupport(
		VkPhysicalDevice device
	) const noexcept;

	void GetQueueSupportInfo(
		VkPhysicalDevice device,
		VkSurfaceKHR surface,
		std::vector<std::pair<size_t, QueueType>>& familyInfos
	) const noexcept;
	void GetSwapchainCapabilities(
		VkPhysicalDevice device,
		VkSurfaceKHR surface,
		SwapChainInfo& details
	) const noexcept;

	void SetQueueFamilyInfo(
		std::vector<std::pair<size_t, QueueType>>& familyInfos
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
