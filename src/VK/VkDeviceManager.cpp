#include <VkDeviceManager.hpp>
#include <ranges>
#include <algorithm>
#include <Exception.hpp>

VkDeviceManager::VkDeviceManager() noexcept
	: m_physicalDevice{ VK_NULL_HANDLE }, m_logicalDevice{ VK_NULL_HANDLE } {}

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

	VkPhysicalDevice suitableDevice = QueryPhysicalDevices(
		devices, surface, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
	);

	if (suitableDevice == VK_NULL_HANDLE)
		suitableDevice = QueryPhysicalDevices(
			devices, surface, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
		);

	if (suitableDevice == VK_NULL_HANDLE)
		throw Exception("QueueFamily Error", "No GPU with all Queue Family support found.");
	else {
		SetQueueFamilyInfo(suitableDevice, surface);
		m_physicalDevice = suitableDevice;
	}
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

void VkDeviceManager::CreateLogicalDevice() {
	size_t mostQueueCount = 0u;
	for (const auto& info : m_usableQueueFamilies)
		mostQueueCount = std::max(mostQueueCount, info.queueRequired);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(std::size(m_usableQueueFamilies));
	const std::vector<float> queuePriorities(mostQueueCount, 1.0f);

	for (size_t index = 0u; index < std::size(queueCreateInfos); ++index) {
		VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos[index];
		const QueueFamilyInfo& queueFamilyInfo = m_usableQueueFamilies[index];

		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = static_cast<std::uint32_t>(queueFamilyInfo.index);
		queueCreateInfo.queueCount = static_cast<std::uint32_t>(queueFamilyInfo.queueRequired);
		queueCreateInfo.pQueuePriorities = std::data(queuePriorities);
	}

	DeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = deviceFeatures.GetDeviceFeatures2();
	createInfo.pQueueCreateInfos = std::data(queueCreateInfos);
	createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(std::size(queueCreateInfos));
	createInfo.enabledExtensionCount = static_cast<std::uint32_t>(std::size(m_extensionNames));
	createInfo.ppEnabledExtensionNames = std::data(m_extensionNames);

	vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);
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

std::pair<VkQueue, std::uint32_t> VkDeviceManager::GetQueue(QueueType type) noexcept {
	VkQueue queue = VK_NULL_HANDLE;
	std::uint32_t familyIndex = 0u;
	for (size_t index = 0u; index < std::size(m_usableQueueFamilies); ++index)
		if (m_usableQueueFamilies[index].typeFlags & type) {
			familyIndex = static_cast<std::uint32_t>(index);

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

void VkDeviceManager::SetQueueFamilyInfo(
	VkPhysicalDevice device, VkSurfaceKHR surface
) {
	FamilyInfo familyInfos;

	// QueueFamily check should have already been done, so familyInfoCheck should return
	// a value for sure
	if (auto familyInfoCheck = QueryQueueFamilyInfo(device, surface); familyInfoCheck)
		familyInfos = std::move(*familyInfoCheck);

	// Sorting family indices to find consecuitive different queue types with same index
	std::ranges::sort(familyInfos,
		[](
			const std::pair<size_t, QueueType>& pair1,
			const std::pair<size_t, QueueType>& pair2
			) { return pair1.first < pair2.first; }
	);

	auto& [previousFamilyIndex, queueType] = familyInfos.front();
	std::uint32_t queueFlag = queueType;
	size_t queueCount = 1u;

	for (size_t index = 1u; index < std::size(familyInfos); ++index) {
		const auto& [familyIndex, familyType] = familyInfos[index];

		if (previousFamilyIndex == familyIndex) {
			++queueCount;
			queueFlag |= familyType;
		}
		else {
			m_usableQueueFamilies.emplace_back(previousFamilyIndex, queueFlag, queueCount, 0u);
			previousFamilyIndex = familyIndex;
			queueFlag = familyType;
			queueCount = 1u;
		}
	}

	m_usableQueueFamilies.emplace_back(previousFamilyIndex, queueFlag, queueCount, 0u);
}

bool VkDeviceManager::IsDeviceSuitable(
	VkPhysicalDevice device, VkSurfaceKHR surface
) const noexcept {
	SurfaceInfo surfaceInfo = QuerySurfaceCapabilities(device, surface);
	FamilyInfo familyInfos;

	if (auto familyInfoCheck = QueryQueueFamilyInfo(device, surface); familyInfoCheck)
		familyInfos = std::move(*familyInfoCheck);
	else
		return false;

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

// Device Features
VkDeviceManager::DeviceFeatures::DeviceFeatures() noexcept
	: m_deviceFeaturesvk1_3{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
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
		.runtimeDescriptorArray = VK_TRUE,
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

VkPhysicalDeviceFeatures2 const* VkDeviceManager::DeviceFeatures::GetDeviceFeatures2(
) const noexcept {
	return &m_deviceFeatures2;
}
