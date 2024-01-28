#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>
#include <VkQueueFamilyManager.hpp>
#include <VkExtensionManager.hpp>
#include <VkFeatureManager.hpp>

class VkDeviceManager {
public:
	VkDeviceManager() noexcept;
	~VkDeviceManager() noexcept;

	VkDeviceManager& FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
	void CreateLogicalDevice(CoreVersion coreVersion);

	[[nodiscard]]
	VkQueueFamilyMananger GetQueueFamilyManager() const noexcept;

	[[nodiscard]]
	inline VkDeviceExtensionManager& ExtensionManager() noexcept
	{ return m_extensionManager; }

	[[nodiscard]]
	VkPhysicalDevice GetPhysicalDevice() const noexcept;
	[[nodiscard]]
	VkDevice GetLogicalDevice() const noexcept;

private:
	[[nodiscard]]
	bool CheckDeviceType(VkPhysicalDevice device,VkPhysicalDeviceType deviceType) const noexcept;
	[[nodiscard]]
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const noexcept;
	[[nodiscard]]
	bool DoesDeviceSupportFeatures(VkPhysicalDevice device) const noexcept;

	[[nodiscard]]
	VkPhysicalDevice QueryPhysicalDevices(
		const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface,
		VkPhysicalDeviceType deviceType
	) const noexcept;
	[[nodiscard]]
	VkPhysicalDevice SelectPhysicalDevice(
		const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface
	) const noexcept;

private:
	VkPhysicalDevice         m_physicalDevice;
	VkDevice                 m_logicalDevice;
	VkQueueFamilyMananger    m_queueFamilyManager;
	VkDeviceExtensionManager m_extensionManager;
};
#endif
