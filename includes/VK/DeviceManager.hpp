#ifndef DEVICE_MANAGER_HPP_
#define DEVICE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkHelperFunctions.hpp>

class DeviceManager {
public:
	using QueueIndicesType = std::vector<std::uint32_t>;

	DeviceManager() noexcept;
	~DeviceManager() noexcept;

	void FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
	void CreateLogicalDevice();

	[[nodiscard]]
	static QueueIndicesType ResolveQueueIndices(
		std::uint32_t index0, std::uint32_t index1, std::uint32_t index2
	) noexcept;
	[[nodiscard]]
	static QueueIndicesType ResolveQueueIndices(
		std::uint32_t index0, std::uint32_t index1
	) noexcept;

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

	[[nodiscard]]
	static QueueIndicesType _resolveQueueIndices(
		std::uint32_t index0, std::uint32_t index1, std::uint32_t index2
	) noexcept;

private:
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;
	const std::vector<const char*> m_extensionNames = {
		"VK_KHR_swapchain",
		"VK_EXT_descriptor_indexing",
		"VK_KHR_shader_draw_parameters"
	};
};
#endif
