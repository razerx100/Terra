#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkHelperFunctions.hpp>

class VkDeviceManager {
public:
	VkDeviceManager() noexcept;
	~VkDeviceManager() noexcept;

	void FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
	void CreateLogicalDevice();

	[[nodiscard]]
	std::pair<VkQueue, std::uint32_t> GetQueue(QueueType type) noexcept;
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
	bool CheckDeviceType(VkPhysicalDevice device,VkPhysicalDeviceType deviceType) const noexcept;
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const noexcept;
	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const noexcept;

	[[nodiscard]]
	VkPhysicalDevice QueryPhysicalDevices(
		const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface,
		VkPhysicalDeviceType deviceType
	) const noexcept;

	void SetQueueFamilyInfo(VkPhysicalDevice device, VkSurfaceKHR surface);

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;
	const std::vector<const char*> m_extensionNames = {
		"VK_KHR_swapchain"
	};

private:
	class DeviceFeatures {
	public:
		DeviceFeatures() noexcept;

		[[nodiscard]]
		VkPhysicalDeviceFeatures2 const* GetDeviceFeatures2() const noexcept;

	private:
		VkPhysicalDeviceVulkan13Features m_deviceFeaturesvk1_3;
		VkPhysicalDeviceVulkan12Features m_deviceFeaturesvk1_2;
		VkPhysicalDeviceVulkan11Features m_deviceFeaturesvk1_1;
		VkPhysicalDeviceFeatures m_deviceFeatures1;
		VkPhysicalDeviceFeatures2 m_deviceFeatures2;
	};
};
#endif
