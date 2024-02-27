#include <VkDeviceManager.hpp>
#include <ranges>
#include <algorithm>
#include <Exception.hpp>
#include <VkHelperFunctions.hpp>
#include <VkFeatureManager.hpp>

VkDeviceManager::VkDeviceManager()
	: m_physicalDevice{ VK_NULL_HANDLE }, m_logicalDevice{ VK_NULL_HANDLE },
	m_queueFamilyManager{}, m_extensionManager{}, m_featureManager{} {}

VkDeviceManager::~VkDeviceManager() noexcept
{
	SelfDestruct();
}

void VkDeviceManager::SelfDestruct() noexcept
{
	if (m_logicalDevice)
		vkDestroyDevice(m_logicalDevice, nullptr);
}

VkDeviceManager& VkDeviceManager::FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
	std::uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (!deviceCount)
		throw Exception("Vulkan Error", "No GPU with Vulkan support.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, std::data(devices));

	VkPhysicalDevice suitableDevice = SelectPhysicalDevice(devices, surface);

	if (suitableDevice == VK_NULL_HANDLE)
		throw Exception("Feature Error", "No GPU with all of the feature-support found.");
	else {
		m_queueFamilyManager.SetQueueFamilyInfo(suitableDevice, surface);
		m_physicalDevice = suitableDevice;
	}

	return *this;
}

VkPhysicalDevice VkDeviceManager::SelectPhysicalDevice(
	const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface
) const noexcept {
	VkPhysicalDevice suitableDevice = QueryPhysicalDevices(
		devices, surface, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
	);

	if (suitableDevice == VK_NULL_HANDLE)
		suitableDevice = QueryPhysicalDevices(
			devices, surface, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
		);

	return suitableDevice;
}

VkPhysicalDevice VkDeviceManager::QueryPhysicalDevices(
	const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface,
	VkPhysicalDeviceType deviceType
) const noexcept {
	for (VkPhysicalDevice device : devices)
		if (CheckDeviceType(device, deviceType))
			if (IsDeviceSuitable(device, surface))
				return device;

	return VK_NULL_HANDLE;
}

VkDeviceManager& VkDeviceManager::SetDeviceFeatures(CoreVersion coreVersion)
{
	const std::vector<DeviceExtension> activeExtensions = m_extensionManager.GetActiveExtensions();
	for (DeviceExtension extension : activeExtensions)
		m_featureManager.SetExtensionFeatures(extension);

	m_featureManager.SetCoreFeatures(coreVersion);

	return *this;
}

void VkDeviceManager::CreateLogicalDevice()
{
	VkQueueFamilyMananger::QueueCreateInfo queueCreateInfo =
		m_queueFamilyManager.GetQueueCreateInfo();

	auto vkDeviceQueueCreateInfo = queueCreateInfo.GetDeviceQueueCreateInfo();

	const std::vector<const char*>& extensionNames = m_extensionManager.GetExtensionNames();

	VkDeviceCreateInfo createInfo
	{
		.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext                   = m_featureManager.GetDeviceFeatures2(),
		.queueCreateInfoCount    = static_cast<std::uint32_t>(std::size(vkDeviceQueueCreateInfo)),
		.pQueueCreateInfos       = std::data(vkDeviceQueueCreateInfo),
		.enabledExtensionCount   = static_cast<std::uint32_t>(std::size(extensionNames)),
		.ppEnabledExtensionNames = std::data(extensionNames)
	};

	vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);

	m_extensionManager.PopulateExtensionFunctions(m_logicalDevice);

	m_queueFamilyManager.CreateQueues(m_logicalDevice);

	m_featureManager.ClearFeatureChecks();
}

bool VkDeviceManager::CheckDeviceType(
	VkPhysicalDevice device, VkPhysicalDeviceType deviceType
) const noexcept {
	VkPhysicalDeviceProperties deviceProperty{};
	vkGetPhysicalDeviceProperties(device, &deviceProperty);

	return deviceProperty.deviceType == deviceType;
}

bool VkDeviceManager::CheckDeviceExtensionSupport(VkPhysicalDevice device) const noexcept
{
	std::uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(
		device, nullptr, &extensionCount, std::data(availableExtensions)
	);

	for (const char* requiredExtension : m_extensionManager.GetExtensionNames())
	{
		bool found = false;
		for (const VkExtensionProperties& extension : availableExtensions)
			if (std::strcmp(requiredExtension, extension.extensionName) == 0)
			{
				found = true;
				break;
			}

		if (!found)
			return false;
	}

	return true;
}

bool VkDeviceManager::IsDeviceSuitable(
	VkPhysicalDevice device, VkSurfaceKHR surface
) const noexcept {

	if (!CheckDeviceExtensionSupport(device))
		return false;

	if (surface != VK_NULL_HANDLE)
	{
		if (!QuerySurfaceCapabilities(device, surface).IsCapable())
			return false;
	}

	if (!DoesDeviceSupportFeatures(device))
		return false;

	if (!VkQueueFamilyMananger::DoesPhysicalDeviceSupportQueues(device, surface))
		return false;

	return true;
}

bool VkDeviceManager::DoesDeviceSupportFeatures(VkPhysicalDevice device) const noexcept
{
	return m_featureManager.CheckFeatureSupport(device);
}
