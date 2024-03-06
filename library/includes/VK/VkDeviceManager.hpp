#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkQueueFamilyManager.hpp>
#include <VkExtensionManager.hpp>
#include <VkFeatureManager.hpp>
#include <SurfaceManager.hpp>

class VkDeviceManager
{
public:
	VkDeviceManager();
	~VkDeviceManager() noexcept;

	VkDeviceManager& SetDeviceFeatures(CoreVersion coreVersion);
	VkDeviceManager& SetPhysicalDevice(VkPhysicalDevice device, const SurfaceManager& surface);
	VkDeviceManager& SetPhysicalDevice(VkPhysicalDevice device);
	VkDeviceManager& SetPhysicalDeviceAutomatic(VkInstance instance, const SurfaceManager& surface);
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
	{ return m_queueFamilyManager; }
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
	bool CheckSurfaceSupport(VkPhysicalDevice device, const SurfaceManager& surface) const noexcept;

	void SelfDestruct() noexcept;

private:
	VkPhysicalDevice         m_physicalDevice;
	VkDevice                 m_logicalDevice;
	VkQueueFamilyMananger    m_queueFamilyManager;
	VkDeviceExtensionManager m_extensionManager;
	VkFeatureManager         m_featureManager;

private:
	template<typename T>
	VkPhysicalDevice SelectPhysicalDeviceAutomatic(
		const std::vector<VkPhysicalDevice>& devices, const T& surface
	) {
		auto GetSuitableDevice = [this]<typename T>
			(const std::vector<VkPhysicalDevice>&devices, const T & surface,
				VkPhysicalDeviceType deviceType) -> VkPhysicalDevice
		{
			for (VkPhysicalDevice device : devices)
				if (CheckDeviceType(device, deviceType))
					if (CheckExtensionAndFeatures(device))
						if constexpr (std::is_same_v<T, SurfaceManager>)
						{
							if (CheckSurfaceSupport(device, surface))
								return device;
						}
						else
							return device;

			return VK_NULL_HANDLE;
		};

		VkPhysicalDevice suitableDevice = GetSuitableDevice(
			devices, surface, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
		);

		if (suitableDevice == VK_NULL_HANDLE)
			suitableDevice = GetSuitableDevice(
				devices, surface, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
			);

		return suitableDevice;
	}

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
		SelfDestruct();

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
