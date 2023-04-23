#include <VkQueueFamilyManager.hpp>

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
