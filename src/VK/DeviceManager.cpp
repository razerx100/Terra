#include <DeviceManager.hpp>
#include <VKThrowMacros.hpp>
#include <algorithm>
#include <InstanceManager.hpp>

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
			std::vector<std::pair<std::uint32_t, QueueType>> familyInfos;
			GetQueueSupportInfo(device, surface, familyInfos);

			if (CheckDeviceExtensionSupport(device)
				&& swapDetails.IsCapable()
				&& !familyInfos.empty()) {

				SetQueueFamilyInfo(familyInfos);
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
				std::vector<std::pair<std::uint32_t, QueueType>> familyInfos;
				GetQueueSupportInfo(device, surface, familyInfos);

				if (CheckDeviceExtensionSupport(device)
					&& swapDetails.IsCapable()
					&& !familyInfos.empty()) {

					SetQueueFamilyInfo(familyInfos);
					m_physicalDevice = device;
					found = true;

					break;
				}
			}

	if (!found)
		VK_GENERIC_THROW("No GPU with all Queue Family support found.");
}

void DeviceManager::CreateLogicalDevice() {
	std::uint32_t mostQueueCount = 0u;
	for (auto& info : m_usableQueueFamilies)
		mostQueueCount = std::max(mostQueueCount, info.queueRequired);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(m_usableQueueFamilies.size());
	std::vector<float> queuePriorities(mostQueueCount, 1.0f);

	for (std::uint32_t index = 0u; index < queueCreateInfos.size(); ++index) {
		queueCreateInfos[index].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[index].queueFamilyIndex = m_usableQueueFamilies[index].index;
		queueCreateInfos[index].queueCount = m_usableQueueFamilies[index].queueRequired;
		queueCreateInfos[index].pQueuePriorities = queuePriorities.data();
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
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

VkDevice DeviceManager::GetLogicalDevice() const noexcept {
	return m_logicalDevice;
}

QueueData DeviceManager::GetQueue(QueueType type) noexcept {
	VkQueue queue;
	std::uint32_t familyIndex = 0u;
	for (std::uint32_t index = 0u; index < m_usableQueueFamilies.size(); ++index)
		if (m_usableQueueFamilies[index].typeFlags & type) {
			familyIndex = index;

			break;
		}

	vkGetDeviceQueue(
		m_logicalDevice, m_usableQueueFamilies[familyIndex].index,
		m_usableQueueFamilies[familyIndex].queueCreated++, &queue
	);

	return { queue, familyIndex };
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

SwapChainInfo DeviceManager::GetSwapChainInfo() const noexcept {
	SwapChainInfo swapInfo;
	GetSwapchainCapabilities(
		m_physicalDevice,
		SurfaceInst::GetRef()->GetSurface(),
		swapInfo
	);

	return swapInfo;
}

void DeviceManager::SetQueueFamilyInfo(
	std::vector<std::pair<std::uint32_t, QueueType>>& familyInfos
) noexcept {
	std::sort(familyInfos.begin(), familyInfos.end(),
		[](
			std::pair<std::uint32_t, QueueType> pair1,
			std::pair<std::uint32_t, QueueType> pair2
			) {
				return pair1.first < pair2.first;
		}
	);

	std::uint32_t lastFamily = familyInfos[0].first;
	std::uint32_t queueCount = 1u;
	std::uint32_t queueFlag = familyInfos[0].second;

	for (std::uint32_t index = 1u; index < familyInfos.size(); ++index) {
		if (lastFamily == familyInfos[index].first) {
			queueCount++;
			queueFlag |= familyInfos[index].second;
		}
		else {
			m_usableQueueFamilies.emplace_back(lastFamily, queueFlag, queueCount, 0u);
			lastFamily = familyInfos[index].first;
			queueFlag = familyInfos[index].second;
			queueCount = 1u;
		}
	}

	m_usableQueueFamilies.emplace_back(lastFamily, queueFlag, queueCount, 0u);
}

void DeviceManager::GetQueueSupportInfo(
	VkPhysicalDevice device,
	VkSurfaceKHR surface,
	std::vector<std::pair<std::uint32_t, QueueType>>& familyInfos
) const noexcept {
	std::uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	bool transfer = false;
	bool compute = false;
	bool present = false;
	bool graphics = false;

	std::vector<std::pair<std::uint32_t, QueueType>> tempData;

	// Transfer only
	for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index)
		if (queueFamilies[index].queueFlags & VK_QUEUE_TRANSFER_BIT
			&& !(queueFamilies[index].queueFlags & 3u)) {
			tempData.emplace_back(index, QueueType::TransferQueue);
			transfer = true;
			queueFamilies[index].queueCount--;

			break;
		}

	if (transfer)
		for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
			if (queueFamilies[index].queueFlags & VK_QUEUE_COMPUTE_BIT
				&& !(queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& queueFamilies[index].queueCount) {
				tempData.emplace_back(index, QueueType::ComputeQueue);
				compute = true;
				queueFamilies[index].queueCount--;

				break;
			}
		}
	else
		for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
			if (queueFamilies[index].queueFlags & VK_QUEUE_COMPUTE_BIT
				&& !(queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& queueFamilies[index].queueCount >= 2) {
				tempData.emplace_back(index, QueueType::TransferQueue);
				tempData.emplace_back(index, QueueType::ComputeQueue);
				compute = true;
				transfer = true;
				queueFamilies[index].queueCount -= 2;

				break;
			}
		}

	if (!transfer)
		for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
			if (queueFamilies[index].queueFlags & VK_QUEUE_TRANSFER_BIT
				&& queueFamilies[index].queueCount) {
				tempData.emplace_back(index, QueueType::TransferQueue);
				transfer = true;
				queueFamilies[index].queueCount--;

				break;
			}
		}

	if (!compute)
		for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
			if (queueFamilies[index].queueFlags & VK_QUEUE_COMPUTE_BIT
				&& queueFamilies[index].queueCount) {
				tempData.emplace_back(index, QueueType::ComputeQueue);
				compute = true;
				queueFamilies[index].queueCount--;

				break;
			}
		}

	for (std::uint32_t index = 0u; index < queueFamilies.size(); ++index) {
		if (queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT
			&& CheckPresentSupport(device, surface, index)
			&& queueFamilies[index].queueCount >= 2) {
			tempData.emplace_back(index, QueueType::GraphicsQueue);
			tempData.emplace_back(index, QueueType::PresentQueue);
			graphics = true;
			present = true;
			queueFamilies[index].queueCount -= 2;

			break;
		}
	}

	if (graphics && present && compute && transfer)
		std::copy(tempData.begin(), tempData.end(), std::back_inserter(familyInfos));
}
