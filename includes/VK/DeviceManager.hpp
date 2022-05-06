#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkHelperFunctions.hpp>

using QueueData = std::pair<VkQueue, size_t>;

class DeviceManager {
public:
	~DeviceManager() noexcept;

	void FindPhysicalDevice(
		VkInstance instance, VkSurfaceKHR surface
	);
	void CreateLogicalDevice();

	QueueData GetQueue(QueueType type) noexcept;

	[[nodiscard]]
	VkPhysicalDevice GetPhysicalDevice() const noexcept;
	[[nodiscard]]
	VkDevice GetLogicalDevice() const noexcept;

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
	bool CheckDeviceExtensionSupport(
		VkPhysicalDevice device
	) const noexcept;
	bool IsDeviceSuitable(
		VkPhysicalDevice device, VkSurfaceKHR surface
	) const noexcept;

	[[nodiscard]]
	VkPhysicalDevice QueryPhysicalDevices(
		const std::vector<VkPhysicalDevice>& devices,
		VkSurfaceKHR surface, VkPhysicalDeviceType deviceType
	) const noexcept;

	void SetQueueFamilyInfo(
		VkPhysicalDevice device, VkSurfaceKHR surface
	) noexcept;

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;
	const std::vector<const char*> m_extensionNames = {
		"VK_KHR_swapchain",
		"VK_EXT_descriptor_indexing"
	};
};
#endif
