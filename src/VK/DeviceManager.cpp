#include <DeviceManager.hpp>
#include <VKThrowMacros.hpp>

DeviceManager::~DeviceManager() noexcept {
	vkDestroyDevice(m_logicalDevice, nullptr);
}

void DeviceManager::CreatePhysicalDevice(
	VkInstance instance,
	VkSurfaceKHR surface
) {
	std::uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (!deviceCount)
		VK_GENERIC_THROW("No GPU with Vulkan support.");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	bool found = false;

	for (VkPhysicalDevice device : devices)
		if (CheckDeviceType(device, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)) {
			SwapChainInfo swapDetails = {};
			GetSwapchainCapabilities(device, surface, swapDetails);
			std::vector<QueueFamilyInfo> familyInfos;
			GetQueueFamilyInfo(device, surface, familyInfos);

			if (CheckDeviceExtensionSupport(device)
				&& swapDetails.IsCapable()
				&& !familyInfos.empty()) {

				m_swapchainInfo = swapDetails;
				m_usableQueueFamilies = familyInfos;
				m_physicalDevice = device;
				found = true;
				break;
			}
		}

	if (!found)
		for (VkPhysicalDevice device : devices)
			if (CheckDeviceType(device, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) {
				SwapChainInfo swapDetails = {};
				GetSwapchainCapabilities(device, surface, swapDetails);
				std::vector<QueueFamilyInfo> familyInfos;
				GetQueueFamilyInfo(device, surface, familyInfos);

				if (CheckDeviceExtensionSupport(device)
					&& swapDetails.IsCapable()
					&& !familyInfos.empty()) {

					m_swapchainInfo = swapDetails;
					m_usableQueueFamilies = familyInfos;
					m_physicalDevice = device;
					found = true;
					break;
				}
			}

	if (!found)
		VK_GENERIC_THROW("No GPU with all Queue Family support found.");
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

bool DeviceManager::CheckPresentSupport(
	VkPhysicalDevice device,
	VkSurfaceKHR surface,
	std::uint32_t index
) const noexcept {
	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);

	return presentSupport;
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

void DeviceManager::GetSwapchainCapabilities(
	VkPhysicalDevice device,
	VkSurfaceKHR surface,
	SwapChainInfo& details
) const noexcept {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	std::uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device, surface, &formatCount, details.formats.data()
		);
	}

	std::uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, surface, &presentModeCount, details.presentModes.data()
		);
	}
}

void DeviceManager::GetQueueFamilyInfo(
	VkPhysicalDevice device,
	VkSurfaceKHR surface,
	std::vector<QueueFamilyInfo>& familyInfos
) const noexcept {
	std::uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index)
		if ((queueFamilies[index].queueFlags & 7u) == 7u
			&& queueFamilies[index].queueCount >= 3u
			&& CheckPresentSupport(device, surface, index)) {
			familyInfos.emplace_back(
				index,
				3u,
				0u,
				queueFamilies[index].queueFlags
			);

			return;
		}

	{
		std::uint32_t remainder = 0u;
		QueueFamilyInfo tempData{};

		for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index)
			if ((queueFamilies[index].queueFlags & 7u) == 6u
				&& queueFamilies[index].queueCount >= 2u) {
				tempData = {
					index,
					2u,
					0u,
					queueFamilies[index].queueFlags
				};

				remainder = 1u;
				break;
			}
			else if ((queueFamilies[index].queueFlags & 7u) == 5u
				&& queueFamilies[index].queueCount >= 2u
				&& CheckPresentSupport(device, surface, index)) {
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
				&& queueFamilies[index].queueCount >= 2u
				&& CheckPresentSupport(device, surface, index)) {
			tempData = {
				index,
				2u,
				0u,
				queueFamilies[index].queueFlags
			};

			remainder = 4u;
			break;
		}

		if (remainder && remainder == 1)
			for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
				if (queueFamilies[index].queueFlags & remainder
					&& queueFamilies[index].queueCount >= 1u
					&& index != tempData.index
					&& CheckPresentSupport(device, surface, index)) {
					familyInfos.emplace_back(tempData);
					familyInfos.emplace_back(
						index,
						1u,
						0u,
						queueFamilies[index].queueFlags
					);

					return;
				}
			}
		else if (remainder)
			for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index)
				if (queueFamilies[index].queueFlags & remainder
					&& queueFamilies[index].queueCount >= 1u
					&& index != tempData.index) {
					familyInfos.emplace_back(tempData);
					familyInfos.emplace_back(
						index,
						1u,
						0u,
						queueFamilies[index].queueFlags
					);

					return;
				}
	}

	std::vector<std::uint32_t> types = { 2u, 4u };
	std::vector<QueueFamilyInfo> tempData;

	for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index)
		if (queueFamilies[index].queueFlags & 1u
			&& CheckPresentSupport(device, surface, index)) {
			tempData.emplace_back(
				index,
				1u,
				0u,
				queueFamilies[index].queueFlags
			);

			break;
		}

	for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
		for (auto typeIt = types.begin(); typeIt != types.end(); ++typeIt)
			if (queueFamilies[index].queueFlags & *typeIt) {
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
			std::copy(tempData.begin(), tempData.end(), familyInfos.begin());

			return;
		}
	}
}
