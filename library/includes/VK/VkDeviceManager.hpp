#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkQueueFamilyManager.hpp>
#include <VkExtensionManager.hpp>
#include <VkFeatureManager.hpp>

class VkDeviceManager
{
public:
	VkDeviceManager();
	~VkDeviceManager() noexcept;

	VkDeviceManager& SetDeviceFeatures(CoreVersion coreVersion);
	VkDeviceManager& FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
	void CreateLogicalDevice();

	[[nodiscard]]
	VkQueueFamilyMananger GetQueueFamilyManager() const noexcept { return m_queueFamilyManager; }
	[[nodiscard]]
	VkDeviceExtensionManager& ExtensionManager() noexcept { return m_extensionManager; }
	[[nodiscard]]
	VkPhysicalDevice GetPhysicalDevice() const noexcept { return m_physicalDevice; }
	[[nodiscard]]
	VkDevice GetLogicalDevice() const noexcept { return m_logicalDevice; }

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
	VkFeatureManager         m_featureManager;

public:
	VkDeviceManager(const VkDeviceManager&) = delete;
	VkDeviceManager& operator=(const VkDeviceManager&) = delete;

	VkDeviceManager(VkDeviceManager&& other) noexcept
		: m_physicalDevice{ other.m_physicalDevice }, m_logicalDevice{ other.m_logicalDevice },
		m_queueFamilyManager{ other.m_queueFamilyManager },
		m_extensionManager{ std::move(other.m_extensionManager) },
		m_featureManager{ std::move(other.m_featureManager) }
	{
		other.m_logicalDevice = VK_NULL_HANDLE;
	}
	VkDeviceManager& operator=(VkDeviceManager&& other) noexcept
	{
		m_physicalDevice      = other.m_physicalDevice;
		m_logicalDevice       = other.m_logicalDevice;
		m_queueFamilyManager  = other.m_queueFamilyManager;
		m_extensionManager    = std::move(other.m_extensionManager);
		m_featureManager      = std::move(other.m_featureManager);
		other.m_logicalDevice = VK_NULL_HANDLE;

		return *this;
	}
};
#endif
