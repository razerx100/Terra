#include <DeviceManager.hpp>
#include <vector>
#include <VKThrowMacros.hpp>

void DeviceManager::CreatePhysicalDevice(VkInstance instance) {
	SetSuitablePhysicalDevice(instance);
}

bool DeviceManager::CheckDeviceType(
	VkPhysicalDevice device,
	VkPhysicalDeviceType deviceType
) const noexcept {
	VkPhysicalDeviceProperties deviceProperty;
	vkGetPhysicalDeviceProperties(device, &deviceProperty);

	return deviceProperty.deviceType == deviceType;
}

bool DeviceManager::CheckQueueFamilySupport(
	VkPhysicalDevice device
) const noexcept {
	std::uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
		bool supported = true;
		for(VkQueueFlagBits familyType : m_queueTypeFlag)
			if ((queueFamily.queueFlags & familyType) == 0) {
				supported = false;
				break;
			}

		if (supported)
			return true;
	}

	return false;
}

void DeviceManager::SetSuitablePhysicalDevice(VkInstance instance) {
	std::uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (!deviceCount)
		VK_GENERIC_THROW("No GPU with Vulkan support.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	bool found = false;

	for (VkPhysicalDevice device : devices)
		if (CheckDeviceType(device, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
			if (CheckQueueFamilySupport(device)) {
				m_physicalDevice = device;
				found = true;
				break;
			}

	if (!found)
		for (VkPhysicalDevice device : devices)
			if (CheckDeviceType(device, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
				if (CheckQueueFamilySupport(device)) {
					m_physicalDevice = device;
					found = true;
					break;
				}

	if (!found)
		VK_GENERIC_THROW("No GPU with all Queue Family support found.");
}

VkPhysicalDevice DeviceManager::GetPhysicalDevice() const noexcept {
	return m_physicalDevice;
}


void DeviceManager::AddQueueTypeCheck(VkQueueFlagBits queueType) noexcept {
	m_queueTypeFlag.emplace_back(queueType);
}
