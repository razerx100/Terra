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
		SetQueueFamilyInfo(suitableDevice, surface);
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
	size_t mostQueueCount = 0u;
	for (const auto& info : m_usableQueueFamilies)
		mostQueueCount = std::max(mostQueueCount, info.queueRequired);

	// This array's element count will be the highest count of queues. In the case of a queue
	// with a lesser count, it will only read the values till its count from the priority array
	// and since those counts will always be lesser, it will always work. Unless you want
	// different priority values for different queues
	const std::vector<float> queuePriorities(mostQueueCount, 1.0f);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (const QueueFamilyInfo& queueFamilyInfo : m_usableQueueFamilies) {
		VkDeviceQueueCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = static_cast<std::uint32_t>(queueFamilyInfo.index),
			.queueCount = static_cast<std::uint32_t>(queueFamilyInfo.queueRequired),
			.pQueuePriorities = std::data(queuePriorities)
		};

		queueCreateInfos.emplace_back(createInfo);
	}

	DeviceFeatures deviceFeatures{};
	if (meshShader)
		deviceFeatures.ActivateMeshShader();

	VkDeviceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = deviceFeatures.GetDeviceFeatures2(),
		.queueCreateInfoCount = static_cast<std::uint32_t>(std::size(queueCreateInfos)),
		.pQueueCreateInfos = std::data(queueCreateInfos),
		.enabledExtensionCount = static_cast<std::uint32_t>(std::size(m_extensionNames)),
		.ppEnabledExtensionNames = std::data(m_extensionNames)
	};

	vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);

	SetQueueFamilyManager();
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

void VkDeviceManager::SetQueueFamilyManager() noexcept {
	auto [graphicsQueue, graphicsIndex] = GetQueue(GraphicsQueue);
	auto [computeQueue, computeIndex] = GetQueue(ComputeQueue);
	auto [transferQueue, transferIndex] = GetQueue(TransferQueue);
	QueueIndices3 indices{
		.transfer = transferIndex,
		.graphics = graphicsIndex,
		.compute = computeIndex,
	};

	m_queueFamilyManager.AddQueueFamilyIndices(indices);
	m_queueFamilyManager.AddQueue(GraphicsQueue, graphicsQueue);
	m_queueFamilyManager.AddQueue(ComputeQueue, computeQueue);
	m_queueFamilyManager.AddQueue(TransferQueue, transferQueue);
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

	// Sorting family indices to find consecuitive different queue types with the same index
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

			QueueFamilyInfo familyInfo{
				.index = previousFamilyIndex,
				.typeFlags = queueFlag,
				.queueRequired = queueCount
			};
			m_usableQueueFamilies.emplace_back(familyInfo);
			previousFamilyIndex = familyIndex;
			queueFlag = familyType;
			queueCount = 1u;
		}
	}

	QueueFamilyInfo familyInfo{
		.index = previousFamilyIndex,
		.typeFlags = queueFlag,
		.queueRequired = queueCount
	};
	m_usableQueueFamilies.emplace_back(familyInfo);
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

	if (!QueryQueueFamilyInfo(device, surface))
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

void VkDeviceManager::ConfigureQueue(
	FamilyInfo& familyInfo, bool& queueTypeAvailability, VkQueueFamilyProperties& queueFamily,
	size_t index, QueueType queueType
) noexcept {
	familyInfo.emplace_back(std::make_pair(index, queueType));
	queueTypeAvailability = true;
	--queueFamily.queueCount;
}

std::optional<VkDeviceManager::FamilyInfo> VkDeviceManager::QueryQueueFamilyInfo(
	VkPhysicalDevice device, VkSurfaceKHR surface
) const noexcept {
	std::uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
		device, &queueFamilyCount, std::data(queueFamilies)
	);

	bool transfer = false;
	bool compute = false;
	bool graphics = false;

	FamilyInfo familyInfo;

	// Transfer only
	for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
		VkQueueFamilyProperties& queueFamily = queueFamilies[index];
		const VkQueueFlags queueFlags = queueFamily.queueFlags;

		if (queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFlags & 3u)) {
			ConfigureQueue(familyInfo, transfer, queueFamily, index, TransferQueue);

			break;
		}
	}

	if (transfer)
		for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];
			const VkQueueFlags queueFlags = queueFamily.queueFlags;

			if (queueFlags & VK_QUEUE_COMPUTE_BIT && !(queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& queueFamily.queueCount) {
				ConfigureQueue(familyInfo, compute, queueFamily, index, ComputeQueue);

				break;
			}
		}
	else
		for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];
			const VkQueueFlags queueFlags = queueFamily.queueFlags;

			if (queueFlags & VK_QUEUE_COMPUTE_BIT && !(queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& queueFamily.queueCount >= 2) {
				familyInfo.emplace_back(std::make_pair(index, TransferQueue));
				familyInfo.emplace_back(std::make_pair(index, ComputeQueue));
				compute = true;
				transfer = true;
				queueFamily.queueCount -= 2;

				break;
			}
		}

	if (!transfer)
		for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];

			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && queueFamily.queueCount) {
				ConfigureQueue(familyInfo, transfer, queueFamily, index, TransferQueue);

				break;
			}
		}

	if (!compute)
		for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];

			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && queueFamily.queueCount) {
				ConfigureQueue(familyInfo, compute, queueFamily, index, ComputeQueue);

				break;
			}
		}

	for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT
			&& CheckPresentSupport(device, surface, index)
			&& queueFamily.queueCount >= 1) {
			ConfigureQueue(familyInfo, graphics, queueFamily, index, GraphicsQueue);

			break;
		}
	}

	if (graphics && compute && transfer)
		return familyInfo;
	else
		return {};
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
