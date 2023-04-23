#include <VkDeviceManager.hpp>
#include <ranges>
#include <algorithm>
#include <Exception.hpp>
#include <VkHelperFunctions.hpp>

VkDeviceManager::VkDeviceManager() noexcept
	: m_physicalDevice{ VK_NULL_HANDLE }, m_logicalDevice{ VK_NULL_HANDLE },
	m_queueFamilyManager{} {}

VkDeviceManager::~VkDeviceManager() noexcept {
	vkDestroyDevice(m_logicalDevice, nullptr);
}

void VkDeviceManager::FindPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
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

void VkDeviceManager::AddExtensionName(const char* name) noexcept {
	m_extensionNames.emplace_back(name);
}

void VkDeviceManager::AddExtensionNames(const std::vector<const char*>& names) noexcept {
	std::ranges::copy(names, std::back_inserter(m_extensionNames));
}

void VkDeviceManager::CreateLogicalDevice(bool meshShader) {
	VkQueueFamilyMananger::QueueCreateInfo queueCreateInfo =
		m_queueFamilyManager.GetQueueCreateInfo();

	auto vkDeviceQueueCreateInfo = queueCreateInfo.GetDeviceQueueCreateInfo();

	DeviceFeatures deviceFeatures{};
	if (meshShader)
		deviceFeatures.ActivateMeshShader();

	VkDeviceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = deviceFeatures.GetDeviceFeatures2(),
		.queueCreateInfoCount = static_cast<std::uint32_t>(std::size(vkDeviceQueueCreateInfo)),
		.pQueueCreateInfos = std::data(vkDeviceQueueCreateInfo),
		.enabledExtensionCount = static_cast<std::uint32_t>(std::size(m_extensionNames)),
		.ppEnabledExtensionNames = std::data(m_extensionNames)
	};

	vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);

	m_queueFamilyManager.CreateQueues(m_logicalDevice);
}

bool VkDeviceManager::CheckDeviceType(
	VkPhysicalDevice device, VkPhysicalDeviceType deviceType
) const noexcept {
	VkPhysicalDeviceProperties deviceProperty{};
	vkGetPhysicalDeviceProperties(device, &deviceProperty);

	return deviceProperty.deviceType == deviceType;
}

VkPhysicalDevice VkDeviceManager::GetPhysicalDevice() const noexcept {
	return m_physicalDevice;
}

VkDevice VkDeviceManager::GetLogicalDevice() const noexcept {
	return m_logicalDevice;
}

VkQueueFamilyMananger VkDeviceManager::GetQueueFamilyManager() const noexcept {
	return m_queueFamilyManager;
}

bool VkDeviceManager::CheckDeviceExtensionSupport(VkPhysicalDevice device) const noexcept {
	std::uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(
		device, nullptr, &extensionCount, std::data(availableExtensions)
	);

	for (const char* requiredExtension : m_extensionNames) {
		bool found = false;
		for (const VkExtensionProperties& extension : availableExtensions)
			if (std::strcmp(requiredExtension, extension.extensionName) == 0) {
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

	if (!QuerySurfaceCapabilities(device, surface).IsCapable())
		return false;

	if (!DoesDeviceSupportFeatures(device))
		return false;

	if (!VkQueueFamilyMananger::DoesPhysicalDeviceSupportQueues(device, surface))
		return false;

	return true;
}

bool VkDeviceManager::DoesDeviceSupportFeatures(VkPhysicalDevice device) const noexcept {
	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES
	};

	VkPhysicalDeviceFeatures2 features2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &indexingFeatures
	};

	vkGetPhysicalDeviceFeatures2(device, &features2);

	return features2.features.samplerAnisotropy
		&& indexingFeatures.descriptorBindingPartiallyBound
		&& indexingFeatures.runtimeDescriptorArray;
}

// Device Features
VkDeviceManager::DeviceFeatures::DeviceFeatures() noexcept
	: m_deviceMeshFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT
	},
	m_deviceFeaturesvk1_3{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &m_deviceMeshFeatures,
		.synchronization2 = VK_TRUE
	},
	m_deviceFeaturesvk1_2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &m_deviceFeaturesvk1_3,
		.drawIndirectCount = VK_TRUE,
		.descriptorIndexing = VK_TRUE,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE
	},
	m_deviceFeaturesvk1_1{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.pNext = &m_deviceFeaturesvk1_2,
		.shaderDrawParameters = VK_TRUE
	},
	m_deviceFeatures1{
		.multiDrawIndirect = VK_TRUE,
		.drawIndirectFirstInstance = VK_TRUE,
		.samplerAnisotropy = VK_TRUE
	},
	m_deviceFeatures2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &m_deviceFeaturesvk1_1,
		.features = m_deviceFeatures1
	} {}

void VkDeviceManager::DeviceFeatures::ActivateMeshShader() noexcept {
	m_deviceMeshFeatures.meshShader =  VK_TRUE;
}

VkPhysicalDeviceFeatures2 const* VkDeviceManager::DeviceFeatures::GetDeviceFeatures2(
) const noexcept {
	return &m_deviceFeatures2;
}
