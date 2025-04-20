#include <VkDeviceManager.hpp>
#include <ranges>
#include <algorithm>
#include <Exception.hpp>
#include <VkFeatureManager.hpp>

VkDeviceManager::VkDeviceManager()
	: m_physicalDevice{ VK_NULL_HANDLE }, m_logicalDevice{ VK_NULL_HANDLE },
	m_queueFamilyManager{ std::make_unique<VkQueueFamilyMananger>() }, m_extensionManager{},
	m_featureManager{}
{}

VkDeviceManager::~VkDeviceManager() noexcept
{
	SelfDestruct();
}

void VkDeviceManager::SelfDestruct() noexcept
{
	if (m_logicalDevice)
		vkDestroyDevice(m_logicalDevice, nullptr);
}

std::vector<VkPhysicalDevice> VkDeviceManager::GetAvailableDevices(VkInstance instance)
{
	std::uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (!deviceCount)
		throw Exception("Vulkan Error", "No GPU with Vulkan support.");

	std::vector<VkPhysicalDevice> devices{ deviceCount };
	vkEnumeratePhysicalDevices(instance, &deviceCount, std::data(devices));

	return devices;
}

VkDeviceManager& VkDeviceManager::SetPhysicalDeviceAutomatic(
	VkInstance instance, VkSurfaceKHR surface
) {
	std::vector<VkPhysicalDevice> devices = GetAvailableDevices(instance);

	VkPhysicalDevice suitableDevice = SelectPhysicalDeviceAutomatic(devices, surface);

	if (suitableDevice == VK_NULL_HANDLE)
		throw Exception("Feature Error", "No GPU with all of the feature-support found.");
	else
	{
		m_queueFamilyManager->SetQueueFamilyInfo(suitableDevice, surface);
		m_physicalDevice = suitableDevice;
	}

	return *this;
}

VkDeviceManager& VkDeviceManager::SetPhysicalDeviceAutomatic(VkInstance instance)
{
	std::vector<VkPhysicalDevice> devices = GetAvailableDevices(instance);

	// This function is used to check feature support only.
	VkPhysicalDevice suitableDevice = SelectPhysicalDeviceAutomatic(devices);

	if (suitableDevice == VK_NULL_HANDLE)
		throw Exception("Feature Error", "No GPU with all of the feature-support found.");
	else
	{
		m_queueFamilyManager->SetQueueFamilyInfo(suitableDevice, VK_NULL_HANDLE);
		m_physicalDevice = suitableDevice;
	}

	return *this;
}

VkDeviceManager& VkDeviceManager::SetPhysicalDevice(
	VkPhysicalDevice device, VkSurfaceKHR surface
) {
	if (!CheckExtensionAndFeatures(device) || !CheckSurfaceSupport(device, surface))
		throw Exception("Feature Error", "No GPU with all of the feature-support found.");

	return *this;
}

VkDeviceManager& VkDeviceManager::SetPhysicalDevice(VkPhysicalDevice device)
{
	if (!CheckExtensionAndFeatures(device))
		throw Exception("Feature Error", "No GPU with all of the feature-support found.");

	return *this;
}

std::vector<VkPhysicalDevice> VkDeviceManager::GetDevicesByType(
	VkInstance instance, VkPhysicalDeviceType type
) {
	std::vector<VkPhysicalDevice> devices = GetAvailableDevices(instance);

	std::erase_if(devices, [type](VkPhysicalDevice device) { return !CheckDeviceType(device, type); });

	return devices;
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
	VkQueueFamilyMananger::QueueCreateInfo queueCreateInfo = m_queueFamilyManager->GetQueueCreateInfo();

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

	m_queueFamilyManager->CreateQueues(m_logicalDevice);

	m_featureManager.ClearFeatureChecks();
}

VkPhysicalDeviceProperties VkDeviceManager::GetDeviceProperties(VkPhysicalDevice device) noexcept
{
	VkPhysicalDeviceProperties deviceProperty{};
	vkGetPhysicalDeviceProperties(device, &deviceProperty);

	return deviceProperty;
}

bool VkDeviceManager::CheckDeviceType(
	VkPhysicalDevice device, VkPhysicalDeviceType deviceType
) noexcept {
	VkPhysicalDeviceProperties deviceProperty = GetDeviceProperties(device);

	return deviceProperty.deviceType == deviceType;
}

bool VkDeviceManager::CheckDeviceExtensionSupport(VkPhysicalDevice device) const noexcept
{
	std::uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions{ extensionCount };
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

bool VkDeviceManager::CheckExtensionAndFeatures(VkPhysicalDevice device) const noexcept
{
	return CheckDeviceExtensionSupport(device) && DoesDeviceSupportFeatures(device);
}

bool VkDeviceManager::CheckSurfaceSupport(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept {
	return VkQueueFamilyMananger::DoesPhysicalDeviceSupportQueues(device, surface)
		&& SurfaceManager::CanDeviceSupportSurface(surface, device);
}

bool VkDeviceManager::DoesDeviceSupportFeatures(VkPhysicalDevice device) const noexcept
{
	return m_featureManager.CheckFeatureSupport(device);
}

VkPhysicalDevice VkDeviceManager::SelectPhysicalDeviceAutomatic(
	const std::vector<VkPhysicalDevice>& devices, VkSurfaceKHR surface
) {
	auto GetSuitableDevice = [this]
		(const std::vector<VkPhysicalDevice>&devices, VkSurfaceKHR surface,
			VkPhysicalDeviceType deviceType) -> VkPhysicalDevice
	{
		for (VkPhysicalDevice device : devices)
			if (CheckDeviceType(device, deviceType))
				if (CheckExtensionAndFeatures(device))
					if (CheckSurfaceSupport(device, surface))
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

VkPhysicalDevice VkDeviceManager::SelectPhysicalDeviceAutomatic(
	const std::vector<VkPhysicalDevice>& devices
) {
	auto GetSuitableDevice = [this]
		(const std::vector<VkPhysicalDevice>&devices, VkPhysicalDeviceType deviceType)
			-> VkPhysicalDevice
	{
		for (VkPhysicalDevice device : devices)
			if (CheckDeviceType(device, deviceType))
				if (CheckExtensionAndFeatures(device))
					return device;

		return VK_NULL_HANDLE;
	};

	VkPhysicalDevice suitableDevice = GetSuitableDevice(
		devices, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
	);

	if (suitableDevice == VK_NULL_HANDLE)
		suitableDevice = GetSuitableDevice(
			devices, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
		);

	return suitableDevice;
}
