#include <VkQueueFamilyManager.hpp>
#include <ranges>
#include <algorithm>
#include <VkHelperFunctions.hpp>

VkQueueFamilyMananger::VkQueueFamilyMananger() noexcept
	: m_queueIndices{}, m_graphicsQueue{ VK_NULL_HANDLE }, m_computeQueue{ VK_NULL_HANDLE },
	m_transferQueue{ VK_NULL_HANDLE } {}

void VkQueueFamilyMananger::AddQueueFamilyIndices(const QueueIndices3& indices) noexcept {
	m_queueIndices = indices;
}

void VkQueueFamilyMananger::AddQueue(QueueType type, VkQueue queue) noexcept {
	switch (type) {
	case GraphicsQueue: {
		m_graphicsQueue = queue;
		break;
	}
	case ComputeQueue: {
		m_computeQueue = queue;
		break;
	}
	case TransferQueue: {
		m_transferQueue = queue;
		break;
	}
	}
}

QueueIndicesCG VkQueueFamilyMananger::GetComputeAndGraphicsIndices() const noexcept {
	return m_queueIndices;
}

QueueIndicesTG VkQueueFamilyMananger::GetTransferAndGraphicsIndices() const noexcept {
	return m_queueIndices;
}

QueueIndices3 VkQueueFamilyMananger::GetAllIndices() const noexcept {
	return m_queueIndices;
}

VkQueue VkQueueFamilyMananger::GetQueue(QueueType type) const noexcept {
	switch (type) {
	case GraphicsQueue:
		return m_graphicsQueue;
	case ComputeQueue:
		return m_computeQueue;
	case TransferQueue:
		return m_transferQueue;
	}

	return VK_NULL_HANDLE;
}

std::uint32_t VkQueueFamilyMananger::GetIndex(QueueType type) const noexcept {
	switch (type) {
	case GraphicsQueue:
		return m_queueIndices.graphics;
	case ComputeQueue:
		return m_queueIndices.compute;
	case TransferQueue:
		return m_queueIndices.transfer;
	}

	return static_cast<std::uint32_t>(-1);
}

void VkQueueFamilyMananger::CreateQueues(VkDevice device) noexcept {
	auto [graphicsQueue, graphicsIndex] = CreateQueue(device, GraphicsQueue);
	auto [computeQueue, computeIndex] = CreateQueue(device, ComputeQueue);
	auto [transferQueue, transferIndex] = CreateQueue(device, TransferQueue);
	QueueIndices3 indices{
		.transfer = transferIndex,
		.graphics = graphicsIndex,
		.compute = computeIndex,
	};

	AddQueueFamilyIndices(indices);
	AddQueue(GraphicsQueue, graphicsQueue);
	AddQueue(ComputeQueue, computeQueue);
	AddQueue(TransferQueue, transferQueue);
}

std::pair<VkQueue, std::uint32_t> VkQueueFamilyMananger::CreateQueue(
	VkDevice device, QueueType type
) noexcept {
	VkQueue queue = VK_NULL_HANDLE;
	std::uint32_t familyIndex = 0u;
	for (size_t index = 0u; index < std::size(m_usableQueueFamilies); ++index)
		if (m_usableQueueFamilies[index].typeFlags & type) {
			familyIndex = static_cast<std::uint32_t>(index);

			break;
		}

	QueueFamilyInfo& queueFamilyInfo = m_usableQueueFamilies[familyIndex];

	vkGetDeviceQueue(
		device, static_cast<std::uint32_t>(queueFamilyInfo.index),
		static_cast<std::uint32_t>(queueFamilyInfo.queueCreated), &queue
	);

	++queueFamilyInfo.queueCreated;

	return { queue, familyIndex };
}

void VkQueueFamilyMananger::SetQueueFamilyInfo(
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

void VkQueueFamilyMananger::ConfigureQueue(
	FamilyInfo& familyInfo, bool& queueTypeAvailability, VkQueueFamilyProperties& queueFamily,
	size_t index, QueueType queueType
) noexcept {
	familyInfo.emplace_back(std::make_pair(index, queueType));
	queueTypeAvailability = true;
	--queueFamily.queueCount;
}

std::optional<VkQueueFamilyMananger::FamilyInfo> VkQueueFamilyMananger::QueryQueueFamilyInfo(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept {
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

bool VkQueueFamilyMananger::DoesPhysicalDeviceSupportQueues(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept {
	return QueryQueueFamilyInfo(device, surface) != std::nullopt;
}

VkQueueFamilyMananger::QueueCreateInfo VkQueueFamilyMananger::GetQueueCreateInfo() const noexcept {
	QueueCreateInfo createInfo{};
	createInfo.SetQueueCreateInfo(m_usableQueueFamilies);

	return createInfo;
}

// QueueCreateInfo
void VkQueueFamilyMananger::QueueCreateInfo::SetPriorities(
	const std::vector<QueueFamilyInfo>& usableQueueFamilies
) noexcept {
	size_t mostQueueCount = 0u;
	for (const auto& info : usableQueueFamilies)
		mostQueueCount = std::max(mostQueueCount, info.queueRequired);

	// This array's element count will be the highest count of queues. In the case of a queue
	// with a lesser count, it will only read the values till its count from the priority array
	// and since those counts will always be lesser, it will always work. Unless you want
	// different priority values for different queues
	m_queuePriorities = std::vector<float>(mostQueueCount, 1.0f);
}

void VkQueueFamilyMananger::QueueCreateInfo::SetQueueCreateInfo(
	const std::vector<QueueFamilyInfo>& usableQueueFamilies
) noexcept {
	SetPriorities(usableQueueFamilies);

	for (const QueueFamilyInfo& queueFamilyInfo : usableQueueFamilies) {
		VkDeviceQueueCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = static_cast<std::uint32_t>(queueFamilyInfo.index),
			.queueCount = static_cast<std::uint32_t>(queueFamilyInfo.queueRequired),
			.pQueuePriorities = std::data(m_queuePriorities)
		};

		m_queueCreateInfo.emplace_back(createInfo);
	}
}

std::vector<VkDeviceQueueCreateInfo> VkQueueFamilyMananger::QueueCreateInfo::GetDeviceQueueCreateInfo(
) const noexcept {
	return m_queueCreateInfo;
}
