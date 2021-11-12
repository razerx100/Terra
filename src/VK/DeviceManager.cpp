#include <DeviceManager.hpp>
#include <VKThrowMacros.hpp>

DeviceManager::~DeviceManager() noexcept {
	vkDestroyDevice(m_logicalDevice, nullptr);
}

void DeviceManager::CreatePhysicalDevice(VkInstance instance) {
	SetSuitablePhysicalDevice(instance);
}

void DeviceManager::CreateLogicalDevice() {
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(m_usableQueueFamilies.size());

	float queuePriority = 1.0f;
	for (std::uint32_t index = 0u; index < m_usableQueueFamilies.size(); ++index) {
		queueCreateInfos[index].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[index].queueFamilyIndex = m_usableQueueFamilies[index].index;
		queueCreateInfos[index].queueCount = m_usableQueueFamilies[index].queueRequired;
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

	for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index)
		if ((queueFamilies[index].queueFlags & 7u)
			== 7u && queueFamilies[index].queueCount >= 3u) {
			m_usableQueueFamilies.emplace_back(
				index,
				3u,
				0u,
				queueFamilies[index].queueFlags
			);

			return true;
		}

	{
		std::uint32_t remainder = 0u;
		QueueFamilyInfo tempData{};

		for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index)
			if ((queueFamilies[index].queueFlags & 7u)
				== 6u && queueFamilies[index].queueCount >= 2u) {
				tempData = {
					index,
					2u,
					0u,
					queueFamilies[index].queueFlags
				};

				remainder = 1u;
				break;
			}
			else if ((queueFamilies[index].queueFlags & 7u)
				== 5u && queueFamilies[index].queueCount >= 2u) {
				tempData = {
					index,
					2u,
					0u,
					queueFamilies[index].queueFlags
				};

				remainder = 2u;
				break;
			}
			else if (std::uint32_t check = (queueFamilies[index].queueFlags & 7u);
				(check == 3u || check == 7u)
				&& queueFamilies[index].queueCount >= 2u) {
				tempData = {
					index,
					2u,
					0u,
					queueFamilies[index].queueFlags
				};

				remainder = 4u;
				break;
			}

		if (remainder)
			for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index)
				if ((queueFamilies[index].queueFlags & remainder)
					!= 0 && queueFamilies[index].queueCount >= 1u && index != tempData.index) {
					m_usableQueueFamilies.emplace_back(tempData);
					m_usableQueueFamilies.emplace_back(
						index,
						1u,
						0u,
						queueFamilies[index].queueFlags
					);

					return true;
				}
	}

	std::vector<std::uint32_t> types = { 1u, 2u, 4u };
	std::vector<QueueFamilyInfo> tempData;

	for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
		for (auto typeIt = types.begin(); typeIt != types.end(); ++typeIt)
			if ((queueFamilies[index].queueFlags & *typeIt)
				!= 0) {
				tempData.emplace_back(
					index,
					1u,
					0u,
					queueFamilies[index].queueFlags
				);
				types.erase(typeIt);

				break;
			}

		if (types.empty()) {
			std::copy(tempData.begin(), tempData.end(), m_usableQueueFamilies.begin());
			return true;
		}
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

std::uint32_t DeviceManager::GetIndexOfQueueFamily(VkQueueFlagBits queueType) const noexcept {
	for (const QueueFamilyInfo& queueFamily : m_usableQueueFamilies)
		if (queueFamily.typeFlags & queueType)
			return queueFamily.index;

	return 0u;
}

VkQueue DeviceManager::GetQueue(VkQueueFlagBits type) noexcept {
	std::uint32_t familyIndex = GetIndexOfQueueFamily(type);
	VkQueue queue;

	vkGetDeviceQueue(
		m_logicalDevice, m_usableQueueFamilies[familyIndex].index,
		m_usableQueueFamilies[familyIndex].queueCreated++, &queue
	);

	return queue;
}
