#include <DeviceManager.hpp>
#include <VKThrowMacros.hpp>
#include <algorithm>

DeviceManager::~DeviceManager() noexcept {
	vkDestroyDevice(m_logicalDevice, nullptr);
}

void DeviceManager::FindPhysicalDevice(
	VkInstance instance, VkSurfaceKHR surface
) {
	std::uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (!deviceCount)
		VK_GENERIC_THROW("No GPU with Vulkan support.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	VkPhysicalDevice suitableDevice = QueryPhysicalDevices(
		devices, surface, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
	);

	if (suitableDevice == VK_NULL_HANDLE)
		suitableDevice = QueryPhysicalDevices(
			devices, surface, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
		);

	if (suitableDevice == VK_NULL_HANDLE)
		VK_GENERIC_THROW("No GPU with all Queue Family support found.");
	else {
		SetQueueFamilyInfo(suitableDevice, surface);
		m_physicalDevice = suitableDevice;
	}
}

VkPhysicalDevice DeviceManager::QueryPhysicalDevices(
	const std::vector<VkPhysicalDevice>& devices,
	VkSurfaceKHR surface, VkPhysicalDeviceType deviceType
) const noexcept {
	for (VkPhysicalDevice device : devices)
		if (CheckDeviceType(device, deviceType))
			if (IsDeviceSuitable(device, surface))
				return device;

	return VK_NULL_HANDLE;
}

void DeviceManager::CreateLogicalDevice() {
	size_t mostQueueCount = 0u;
	for (const auto& info : m_usableQueueFamilies)
		mostQueueCount = std::max(mostQueueCount, info.queueRequired);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(m_usableQueueFamilies.size());
	const std::vector<float> queuePriorities(mostQueueCount, 1.0f);

	for (size_t index = 0u; index < queueCreateInfos.size(); ++index) {
		VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos[index];
		const QueueFamilyInfo& queueFamilyInfo = m_usableQueueFamilies[index];

		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = static_cast<std::uint32_t>(queueFamilyInfo.index);
		queueCreateInfo.queueCount = static_cast<std::uint32_t>(queueFamilyInfo.queueRequired);
		queueCreateInfo.pQueuePriorities = queuePriorities.data();
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	indexingFeatures.runtimeDescriptorArray = VK_TRUE;
	indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features = deviceFeatures;
	deviceFeatures2.pNext = &indexingFeatures;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
	createInfo.enabledExtensionCount = static_cast<std::uint32_t>(m_extensionNames.size());
	createInfo.ppEnabledExtensionNames = m_extensionNames.data();
	createInfo.pNext = &deviceFeatures2;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice)
	);
}

bool DeviceManager::CheckDeviceType(
	VkPhysicalDevice device, VkPhysicalDeviceType deviceType
) const noexcept {
	VkPhysicalDeviceProperties deviceProperty;
	vkGetPhysicalDeviceProperties(device, &deviceProperty);

	return deviceProperty.deviceType == deviceType;
}

VkPhysicalDevice DeviceManager::GetPhysicalDevice() const noexcept {
	return m_physicalDevice;
}

VkDevice DeviceManager::GetLogicalDevice() const noexcept {
	return m_logicalDevice;
}

QueueData DeviceManager::GetQueue(QueueType type) noexcept {
	VkQueue queue;
	size_t familyIndex = 0u;
	for (size_t index = 0u; index < m_usableQueueFamilies.size(); ++index)
		if (m_usableQueueFamilies[index].typeFlags & type) {
			familyIndex = index;

			break;
		}

	QueueFamilyInfo& queueFamilyInfo = m_usableQueueFamilies[familyIndex];

	vkGetDeviceQueue(
		m_logicalDevice, static_cast<std::uint32_t>(queueFamilyInfo.index),
		static_cast<std::uint32_t>(queueFamilyInfo.queueCreated), &queue
	);

	++queueFamilyInfo.queueCreated;

	return { queue, familyIndex };
}

bool DeviceManager::CheckDeviceExtensionSupport(
	VkPhysicalDevice device
) const noexcept {
	std::uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(
		device, nullptr, &extensionCount, availableExtensions.data()
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

void DeviceManager::SetQueueFamilyInfo(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept {
	FamilyInfo familyInfos = QueryQueueFamilyInfo(device, surface);

	std::sort(familyInfos.begin(), familyInfos.end(),
		[](
			std::pair<size_t, QueueType> pair1,
			std::pair<size_t, QueueType> pair2
			) {
				return pair1.first < pair2.first;
		}
	);

	auto [lastFamily, queueType] = familyInfos[0];
	std::uint32_t queueFlag = queueType;
	size_t queueCount = 1u;

	for (size_t index = 1u; index < familyInfos.size(); ++index) {
		const auto [familyIndex, familyType] = familyInfos[index];

		if (lastFamily == familyIndex) {
			++queueCount;
			queueFlag |= familyType;
		}
		else {
			m_usableQueueFamilies.emplace_back(lastFamily, queueFlag, queueCount, 0u);
			lastFamily = familyIndex;
			queueFlag = familyType;
			queueCount = 1u;
		}
	}

	m_usableQueueFamilies.emplace_back(lastFamily, queueFlag, queueCount, 0u);
}

bool DeviceManager::IsDeviceSuitable(
	VkPhysicalDevice device, VkSurfaceKHR surface
) const noexcept {
	SurfaceInfo surfaceInfo = QuerySurfaceCapabilities(device, surface);
	FamilyInfo familyInfos = QueryQueueFamilyInfo(device, surface);

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

	VkPhysicalDeviceFeatures2 features2 = {};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = &indexingFeatures;

	vkGetPhysicalDeviceFeatures2(device, &features2);

	if (CheckDeviceExtensionSupport(device)
		&& surfaceInfo.IsCapable()
		&& !std::empty(familyInfos)
		&& features2.features.samplerAnisotropy
		&& indexingFeatures.descriptorBindingPartiallyBound
		&& indexingFeatures.runtimeDescriptorArray)
		return true;
	else
		return false;
}
