#ifndef VK_QUEUE_FAMILY_MANAGER_HPP_
#define VK_QUEUE_FAMILY_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <cstdint>

enum QueueType {
	TransferQueue = 1,
	ComputeQueue = 2,
	GraphicsQueue = 4
};

struct QueueIndicesTG {
	std::uint32_t transfer;
	std::uint32_t graphics;
};

struct QueueIndicesCG {
	std::uint32_t compute;
	std::uint32_t graphics;
};

struct QueueIndices3 {
	std::uint32_t transfer;
	std::uint32_t graphics;
	std::uint32_t compute;

	inline operator QueueIndicesTG() const {
		return { transfer, graphics };
	}

	inline operator QueueIndicesCG() const {
		return { compute, graphics };
	}
};

class VkQueueFamilyMananger {
	friend class VkDeviceManager;
public:
	VkQueueFamilyMananger() noexcept;

	[[nodiscard]]
	QueueIndicesCG GetComputeAndGraphicsIndices() const noexcept;
	[[nodiscard]]
	QueueIndicesTG GetTransferAndGraphicsIndices() const noexcept;
	[[nodiscard]]
	QueueIndices3 GetAllIndices() const noexcept;
	[[nodiscard]]
	std::uint32_t GetIndex(QueueType type) const noexcept;

	[[nodiscard]]
	VkQueue GetQueue(QueueType type) const noexcept;

private:
	void AddQueueFamilyIndices(const QueueIndices3& indices) noexcept;
	void AddQueue(QueueType type, VkQueue queue) noexcept;

private:
	QueueIndices3 m_queueIndices;
	VkQueue m_graphicsQueue;
	VkQueue m_computeQueue;
	VkQueue m_transferQueue;
};
#endif
