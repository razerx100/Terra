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

	operator QueueIndicesTG() const { return { transfer, graphics }; }

	operator QueueIndicesCG() const { return { compute, graphics }; }
};

class VkQueueFamilyMananger
{
	friend class VkDeviceManager;
public:
	VkQueueFamilyMananger() noexcept;

	[[nodiscard]]
	QueueIndicesCG GetComputeAndGraphicsIndices() const noexcept { return GetAllIndices(); }
	[[nodiscard]]
	QueueIndicesTG GetTransferAndGraphicsIndices() const noexcept { return GetAllIndices(); }
	[[nodiscard]]
	QueueIndices3 GetAllIndices() const noexcept { return m_queueIndices; }
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
		size_t index            = 0u;
		size_t queueRequired    = 0u;
		size_t queueCreated     = 0u;
		std::uint32_t typeFlags = 0u;
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

	[[nodiscard]]
	static bool CheckPresentSupport(
		VkPhysicalDevice device, VkSurfaceKHR surface, size_t index
	) noexcept;

private:
	QueueIndices3                m_queueIndices;
	VkQueue                      m_graphicsQueue;
	VkQueue                      m_computeQueue;
	VkQueue                      m_transferQueue;
	std::vector<QueueFamilyInfo> m_usableQueueFamilies;

private:
	class QueueCreateInfo
	{
	public:
		void SetQueueCreateInfo(
			const std::vector<QueueFamilyInfo>& usableQueueFamilies
		) noexcept;

		[[nodiscard]]
		std::vector<VkDeviceQueueCreateInfo> GetDeviceQueueCreateInfo() const noexcept
		{
			return m_queueCreateInfo;
		}

	private:
		void SetPriorities(const std::vector<QueueFamilyInfo>& usableQueueFamilies) noexcept;

	private:
		std::vector<float>                   m_queuePriorities;
		std::vector<VkDeviceQueueCreateInfo> m_queueCreateInfo;
	};

public:
	VkQueueFamilyMananger(const VkQueueFamilyMananger& other) noexcept
		: m_queueIndices{ other.m_queueIndices }, m_graphicsQueue{ other.m_graphicsQueue },
		m_computeQueue{ other.m_computeQueue }, m_transferQueue{ other.m_transferQueue },
		m_usableQueueFamilies{ other.m_usableQueueFamilies }
	{}
	VkQueueFamilyMananger& operator=(const VkQueueFamilyMananger& other) noexcept
	{
		m_queueIndices        = other.m_queueIndices;
		m_graphicsQueue       = other.m_graphicsQueue;
		m_computeQueue        = other.m_computeQueue;
		m_transferQueue       = other.m_transferQueue;
		m_usableQueueFamilies = other.m_usableQueueFamilies;

		return *this;
	}

	VkQueueFamilyMananger(VkQueueFamilyMananger&& other) noexcept
		: m_queueIndices{ other.m_queueIndices }, m_graphicsQueue{ other.m_graphicsQueue },
		m_computeQueue{ other.m_computeQueue }, m_transferQueue{ other.m_transferQueue },
		m_usableQueueFamilies{ std::move(other.m_usableQueueFamilies) }
	{}
	VkQueueFamilyMananger& operator=(VkQueueFamilyMananger&& other) noexcept
	{
		m_queueIndices        = other.m_queueIndices;
		m_graphicsQueue       = other.m_graphicsQueue;
		m_computeQueue        = other.m_computeQueue;
		m_transferQueue       = other.m_transferQueue;
		m_usableQueueFamilies = std::move(other.m_usableQueueFamilies);

		return *this;
	}
};
#endif
