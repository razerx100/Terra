#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>
#include <VkQueueFamilyManager.hpp>

class VkDeviceManager {
public:
	VkDeviceManager() noexcept;
	~VkDeviceManager() noexcept;

	void FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
	void CreateLogicalDevice(bool meshShader);
	void AddExtensionName(const char* name) noexcept;
	void AddExtensionNames(const std::vector<const char*>& names) noexcept;

	[[nodiscard]]
	VkQueueFamilyMananger GetQueueFamilyManager() const noexcept;

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

	using FamilyInfo = std::vector<std::pair<size_t, QueueType>>;

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
	std::pair<VkQueue, std::uint32_t> GetQueue(QueueType type) noexcept;
	[[nodiscard]]
	std::optional<FamilyInfo> QueryQueueFamilyInfo(
		VkPhysicalDevice device, VkSurfaceKHR surface
	) const noexcept;
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
	void SetQueueFamilyManager() noexcept;

	static void ConfigureQueue(
		FamilyInfo& familyInfo, bool& queueTypeAvailability, VkQueueFamilyProperties& queueFamily,
		size_t index, QueueType queueType
	) noexcept;

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;
	std::vector<const char*> m_extensionNames = {
		"VK_KHR_swapchain"
	};
	VkQueueFamilyMananger m_queueFamilyManager;

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
