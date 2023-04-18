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
	void CreateLogicalDevice(bool meshShader);
	void AddExtensionName(const char* name) noexcept;
	void AddExtensionNames(const std::vector<const char*>& names) noexcept;

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

	void SetQueueFamilyInfo(VkPhysicalDevice device, VkSurfaceKHR surface);

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;
	std::vector<const char*> m_extensionNames = {
		"VK_KHR_swapchain"
	};

private:
	class DeviceFeatures {
	public:
		DeviceFeatures() noexcept;

		void ActivateMeshShader() noexcept;
		[[nodiscard]]
		VkPhysicalDeviceFeatures2 const* GetDeviceFeatures2() const noexcept;

	private:
		VkPhysicalDeviceMeshShaderFeaturesEXT m_deviceMeshFeatures;
		VkPhysicalDeviceVulkan13Features m_deviceFeaturesvk1_3;
		VkPhysicalDeviceVulkan12Features m_deviceFeaturesvk1_2;
		VkPhysicalDeviceVulkan11Features m_deviceFeaturesvk1_1;
		VkPhysicalDeviceFeatures m_deviceFeatures1;
		VkPhysicalDeviceFeatures2 m_deviceFeatures2;
	};
};
#endif
