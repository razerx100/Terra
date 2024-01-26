#ifndef VK_QUEUE_FAMILY_MANAGER_HPP_
#define VK_QUEUE_FAMILY_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <optional>

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
	static bool DoesPhysicalDeviceSupportQueues(
		VkPhysicalDevice device, VkSurfaceKHR surface
	) noexcept;

	[[nodiscard]]
	VkQueue GetQueue(QueueType type) const noexcept;

private:
	struct QueueFamilyInfo {
		size_t index = 0u;
		std::uint32_t typeFlags = 0u;
		size_t queueRequired = 0u;
		size_t queueCreated = 0u;
	};

	using FamilyInfo = std::vector<std::pair<size_t, QueueType>>;
	class QueueCreateInfo;

private:
	void AddQueueFamilyIndices(const QueueIndices3& indices) noexcept;
	void AddQueue(QueueType type, VkQueue queue) noexcept;

	[[nodiscard]]
	std::pair<VkQueue, std::uint32_t> CreateQueue(VkDevice device, QueueType type) noexcept;
	[[nodiscard]]
	static std::optional<FamilyInfo> QueryQueueFamilyInfo(
		VkPhysicalDevice device, VkSurfaceKHR surface
	) noexcept;
	[[nodiscard]]
	QueueCreateInfo GetQueueCreateInfo() const noexcept;

	void SetQueueFamilyInfo(VkPhysicalDevice device, VkSurfaceKHR surface);
	void CreateQueues(VkDevice device) noexcept;

	static void ConfigureQueue(
		FamilyInfo& familyInfo, bool& queueTypeAvailability,
		VkQueueFamilyProperties& queueFamily, size_t index, QueueType queueType
	) noexcept;

private:
	QueueIndices3 m_queueIndices;
	VkQueue m_graphicsQueue;
	VkQueue m_computeQueue;
	VkQueue m_transferQueue;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;

private:
	class QueueCreateInfo {
	public:
		void SetQueueCreateInfo(
			const std::vector<QueueFamilyInfo>& usableQueueFamilies
		) noexcept;

		[[nodiscard]]
		std::vector<VkDeviceQueueCreateInfo> GetDeviceQueueCreateInfo() const noexcept;

	private:
		void SetPriorities(const std::vector<QueueFamilyInfo>& usableQueueFamilies) noexcept;

	private:
		std::vector<float> m_queuePriorities;
		std::vector<VkDeviceQueueCreateInfo> m_queueCreateInfo;
	};
};
#endif
