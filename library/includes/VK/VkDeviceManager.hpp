#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkQueueFamilyManager.hpp>
#include <VkExtensionManager.hpp>
#include <VkFeatureManager.hpp>
#include <VkSurfaceManager.hpp>

class VkDeviceManager
{
public:
	VkDeviceManager();
	~VkDeviceManager() noexcept;

	VkDeviceManager& SetDeviceFeatures(CoreVersion coreVersion);

	VkDeviceManager& SetPhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkDeviceManager& SetPhysicalDevice(VkPhysicalDevice device);

	VkDeviceManager& SetPhysicalDeviceAutomatic(VkInstance instance, VkSurfaceKHR surface);
	VkDeviceManager& SetPhysicalDeviceAutomatic(VkInstance instance);

	void CreateLogicalDevice();

	[[nodiscard]]
	static std::vector<VkPhysicalDevice> GetAvailableDevices(VkInstance instance);
	[[nodiscard]]
	static std::vector<VkPhysicalDevice> GetDevicesByType(
		VkInstance instance, VkPhysicalDeviceType type
	);

	[[nodiscard]]
	VkPhysicalDeviceProperties GetCurrentDeviceProperties() const noexcept
	{
		return GetDeviceProperties(m_physicalDevice);
	}
	[[nodiscard]]
	static VkPhysicalDeviceProperties GetDeviceProperties(VkPhysicalDevice device) noexcept;

	[[nodiscard]]
	const VkQueueFamilyMananger& GetQueueFamilyManager() const noexcept
	{
		return *m_queueFamilyManager;
	}
	[[nodiscard]]
	VkQueueFamilyMananger const* GetQueueFamilyManagerRef() const noexcept
	{
		return m_queueFamilyManager.get();
	}
	[[nodiscard]]
	VkDeviceExtensionManager& ExtensionManager() noexcept { return m_extensionManager; }
	[[nodiscard]]
	VkPhysicalDevice GetPhysicalDevice() const noexcept { return m_physicalDevice; }
	[[nodiscard]]
	VkDevice GetLogicalDevice() const noexcept { return m_logicalDevice; }

private:
	[[nodiscard]]
	static bool CheckDeviceType(VkPhysicalDevice device,VkPhysicalDeviceType deviceType) noexcept;
	[[nodiscard]]
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	bool DoesDeviceSupportFeatures(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	bool CheckExtensionAndFeatures(VkPhysicalDevice device) const noexcept;

	[[nodiscard]]
	static bool CheckSurfaceSupport(VkPhysicalDevice device, VkSurfaceKHR surface) noexcept;

	void SelfDestruct() noexcept;

private:
	VkPhysicalDevice                       m_physicalDevice;
	VkDevice                               m_logicalDevice;
	// The pointer to this is shared in different places. So, if I make it a automatic
	// member, the kept pointers would be invalid after a move.
	std::unique_ptr<VkQueueFamilyMananger> m_queueFamilyManager;
	VkDeviceExtensionManager               m_extensionManager;
	VkFeatureManager                       m_featureManager;

private:
	[[nodiscard]]
	VkPhysicalDevice SelectPhysicalDeviceAutomatic(
		const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface
	);
	[[nodiscard]]
	VkPhysicalDevice SelectPhysicalDeviceAutomatic(const std::vector<VkPhysicalDevice>& devices);

public:
	VkDeviceManager(const VkDeviceManager&) = delete;
	VkDeviceManager& operator=(const VkDeviceManager&) = delete;

	VkDeviceManager(VkDeviceManager&& other) noexcept
		: m_physicalDevice{ other.m_physicalDevice },
		m_logicalDevice{ std::exchange(other.m_logicalDevice, VK_NULL_HANDLE) },
		m_queueFamilyManager{ std::move(other.m_queueFamilyManager) },
		m_extensionManager{ std::move(other.m_extensionManager) },
		m_featureManager{ std::move(other.m_featureManager) }
	{}
	VkDeviceManager& operator=(VkDeviceManager&& other) noexcept
	{
		SelfDestruct();

		m_physicalDevice     = other.m_physicalDevice;
		m_logicalDevice      = std::exchange(other.m_logicalDevice, VK_NULL_HANDLE);
		m_queueFamilyManager = std::move(other.m_queueFamilyManager);
		m_extensionManager   = std::move(other.m_extensionManager);
		m_featureManager     = std::move(other.m_featureManager);

		return *this;
	}
};
#endif
