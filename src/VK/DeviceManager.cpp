#include <DeviceManager.hpp>
#include <VKThrowMacros.hpp>

DeviceManager::~DeviceManager() noexcept {
	vkDestroyDevice(m_logicalDevice, nullptr);
}

void DeviceManager::CreatePhysicalDevice(VkInstance instance) {
	SetSuitablePhysicalDevice(instance);
}

void DeviceManager::CreateLogicalDevice() {
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(m_availableQueueFamily.size());

	float queuePriority = 1.0f;
	for (std::uint32_t index = 0u; index < m_availableQueueFamily.size(); ++index) {
		queueCreateInfos[index].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[index].queueFamilyIndex = m_availableQueueFamily[index].index;
		queueCreateInfos[index].queueCount = m_availableQueueFamily[index].queueRequired;
		queueCreateInfos[index].pQueuePriorities = &queuePriority;
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<std::uint32_t>(m_extensionNames.size());
	createInfo.ppEnabledExtensionNames = m_extensionNames.data();

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice)
	);
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
) noexcept {
	std::uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	bool graphicsQueue = false;
	bool computeQueue = false;
	bool transferQueue = false;

	m_availableQueueFamily.emplace_back();

	for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
		CheckQueueTypeSupport(
			graphicsQueue,
			VK_QUEUE_GRAPHICS_BIT,
			index,
			queueFamilies[index]
		);

		CheckQueueTypeSupport(
			computeQueue,
			VK_QUEUE_COMPUTE_BIT,
			index,
			queueFamilies[index]
		);

		CheckQueueTypeSupport(
			transferQueue,
			VK_QUEUE_TRANSFER_BIT,
			index,
			queueFamilies[index]
		);

		if (graphicsQueue && computeQueue && transferQueue)
			return true;
	}

	m_availableQueueFamily = std::vector<QueueFamilyInfo>();

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

std::uint32_t DeviceManager::GetIndexOfQueueFamily(VkQueueFlagBits queueType) const noexcept {
	for (const QueueFamilyInfo& queueFamily : m_availableQueueFamily)
		if (queueFamily.typeFlags & queueType)
			return queueFamily.index;
}

VkQueue DeviceManager::GetQueue(VkQueueFlagBits type) noexcept {
	std::uint32_t familyIndex = GetIndexOfQueueFamily(type);
	VkQueue queue;

	vkGetDeviceQueue(
		m_logicalDevice, m_availableQueueFamily[familyIndex].index,
		m_availableQueueFamily[familyIndex].queueCreated++, &queue
	);

	return queue;
}

void DeviceManager::CheckQueueTypeSupport(
	bool& checkFlag, VkQueueFlagBits queueType,
	std::uint32_t index,
	const VkQueueFamilyProperties& familyProperty
) noexcept {
	if (!checkFlag)
		if (
			familyProperty.queueFlags & queueType &&
			familyProperty.queueCount > m_availableQueueFamily.back().queueRequired
			) {
			if (m_availableQueueFamily.back().index != index) {
				m_availableQueueFamily.emplace_back();
				m_availableQueueFamily.back().index = index;
			}

			m_availableQueueFamily.back().typeFlags |= queueType;
			m_availableQueueFamily.back().queueRequired++;
			checkFlag = true;
		}
}
