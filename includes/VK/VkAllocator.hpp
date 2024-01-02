#ifndef VK_ALLOCATOR_HPP_
#define VK_ALLOCATOR_HPP_
#include <MemoryTree.hpp>
#include <DeviceMemory.hpp>
#include <optional>
#include <queue>

class VkAllocator
{
public:
	VkAllocator(DeviceMemory2&& memory, size_t id);

	[[nodiscard]]
	// Returns false if there is not enough memory.
	std::optional<VkDeviceSize> AllocateBuffer(
		VkDevice device, const VkMemoryRequirements& memoryReq, VkBuffer buffer
	) noexcept;
	[[nodiscard]]
	// Returns false if there is not enough memory.
	std::optional<VkDeviceSize> AllocateImage(
		VkDevice device, const VkMemoryRequirements& memoryReq, VkImage image
	) noexcept;

	void Deallocate(VkDeviceSize startingAddress, VkDeviceSize bufferSize) noexcept;

	[[nodiscard]]
	inline size_t GetID() const noexcept { return m_id; }

	[[nodiscard]]
	inline VkDeviceSize Size() const noexcept { return m_memory.Size(); }
	[[nodiscard]]
	inline VkDeviceSize AvailableSize() const noexcept
	{ return static_cast<VkDeviceSize>(m_allocator.AvailableSize()); }
	[[nodiscard]]
	inline std::uint8_t* GetCPUStart() const noexcept { return m_memory.CPUMemory(); }

private:
	[[nodiscard]]
	std::optional<VkDeviceSize> Allocate(const VkMemoryRequirements& memoryReq) noexcept;

private:
	DeviceMemory2 m_memory;
	MemoryTree    m_allocator;
	size_t        m_id;

public:
	VkAllocator(const VkAllocator&) = delete;
	VkAllocator& operator=(const VkAllocator&) = delete;

	VkAllocator(VkAllocator&& other) noexcept
		: m_memory{ std::move(other.m_memory) }, m_allocator{ std::move(other.m_allocator) },
		m_id{ other.m_id } {}

	VkAllocator& operator=(VkAllocator&& other) noexcept
	{
		m_memory    = std::move(other.m_memory);
		m_allocator = std::move(other.m_allocator);
		m_id        = other.m_id;

		return *this;
	}
};

class MemoryManager
{
public:
	struct MemoryAllocation
	{
		VkDeviceSize  gpuOffset;
		std::uint8_t* cpuOffset;
		std::size_t   memoryID;
		VkDeviceSize  size;
	};

public:
	MemoryManager(
		VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize initialBudgetGPU,
		VkDeviceSize initialBudgetCPU
	);

	[[nodiscard]]
	MemoryAllocation AllocateBuffer(VkBuffer buffer, VkMemoryPropertyFlagBits memoryType);
	[[nodiscard]]
	MemoryAllocation AllocateImage(VkImage image, VkMemoryPropertyFlagBits memoryType);

	void Deallocate(const MemoryAllocation& allocation, VkMemoryPropertyFlagBits memoryType) noexcept;

private:
	struct MemoryType
	{
		std::uint32_t            index;
		VkMemoryPropertyFlagBits type;
	};

private:
	[[nodiscard]]
	DeviceMemory2 CreateMemory(VkDeviceSize size, MemoryType memoryType) const;

	[[nodiscard]]
	std::optional<MemoryType> GetMemoryType(
		VkDeviceSize size, VkMemoryPropertyFlagBits memoryType
	) const noexcept;

	[[nodiscard]]
	VkDeviceSize GetAvailableMemoryOfType(VkMemoryPropertyFlagBits memoryType) const noexcept;

	[[nodiscard]]
	VkDeviceSize GetNewAllocationSize(VkMemoryPropertyFlagBits memoryType) const noexcept;

	// Since this is a private function and should only be accessed in one translation unit, it
	// should be fine to have it defined in the cpp.
	template<typename T>
	[[nodiscard]]
	MemoryAllocation Allocate(T resource, VkMemoryPropertyFlagBits memoryType);

	[[nodiscard]]
	size_t GetID(bool cpu) noexcept;

private:
	VkDevice                 m_logicalDevice;
	VkPhysicalDevice         m_physicalDevice;
	std::vector<VkAllocator> m_cpuAllocators;
	std::vector<VkAllocator> m_gpuAllocators;
	std::queue<size_t>       m_availableGPUIndices;
	std::queue<size_t>       m_availableCPUIndices;

public:
	MemoryManager(const MemoryManager&) = delete;
	MemoryManager& operator=(const MemoryManager&) = delete;

	MemoryManager(MemoryManager&& other) noexcept
		: m_logicalDevice{ other.m_logicalDevice }, m_physicalDevice{ other.m_physicalDevice },
		m_cpuAllocators{ std::move(other.m_cpuAllocators) },
		m_gpuAllocators{ std::move(other.m_gpuAllocators) },
		m_availableGPUIndices{ std::move(other.m_availableGPUIndices) },
	    m_availableCPUIndices{ std::move(other.m_availableCPUIndices) } {}

	MemoryManager& operator=(MemoryManager&& other) noexcept
	{
		m_logicalDevice       = other.m_logicalDevice;
		m_physicalDevice      = other.m_physicalDevice;
		m_cpuAllocators       = std::move(other.m_cpuAllocators);
		m_gpuAllocators       = std::move(other.m_gpuAllocators);
		m_availableGPUIndices = std::move(other.m_availableGPUIndices);
		m_availableCPUIndices = std::move(other.m_availableCPUIndices);

		return *this;
	}
};
#endif
