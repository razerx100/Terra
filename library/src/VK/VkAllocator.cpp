#include <VkAllocator.hpp>
#include <concepts>
#include <cmath>
#include <algorithm>
#include <Exception.hpp>

[[nodiscard]]
static VkMemoryRequirements GetMemoryRequirements(VkDevice device, VkBuffer buffer) noexcept
{
	VkMemoryRequirements memReq{};
	vkGetBufferMemoryRequirements(device, buffer, &memReq);

	return memReq;
}

[[nodiscard]]
static VkMemoryRequirements GetMemoryRequirements(VkDevice device, VkImage image) noexcept
{
	VkMemoryRequirements memReq{};
	vkGetImageMemoryRequirements(device, image, &memReq);

	return memReq;
}

// VkAllocator
VkAllocator::VkAllocator(DeviceMemory2&& memory, std::uint16_t id)
	: m_memory{ std::move(memory) }, m_allocator{ 0u, m_memory.Size(), 256_B }, m_id{ id } {}

std::optional<VkDeviceSize> VkAllocator::Allocate(const VkMemoryRequirements& memoryReq) noexcept
{
	std::optional<size_t> allocationStart = m_allocator.AllocateN(
		static_cast<size_t>(memoryReq.size), static_cast<size_t>(memoryReq.alignment)
	);

	if (allocationStart)
		return static_cast<VkDeviceSize>(allocationStart.value());

	return {};
}

std::optional<VkDeviceSize> VkAllocator::AllocateBuffer(
	VkDevice device, const VkMemoryRequirements& memoryReq, VkBuffer buffer
) noexcept {
	std::optional<VkDeviceSize> allocationStart = Allocate(memoryReq);

	if (allocationStart)
		vkBindBufferMemory(device, buffer, m_memory.Memory(), allocationStart.value());

	return allocationStart;
}

std::optional<VkDeviceSize> VkAllocator::AllocateImage(
	VkDevice device, const VkMemoryRequirements& memoryReq, VkImage image
) noexcept
{
	std::optional<VkDeviceSize> allocationStart = Allocate(memoryReq);

	if (allocationStart)
		vkBindImageMemory(device, image, m_memory.Memory(), allocationStart.value());

	return allocationStart;
}

void VkAllocator::Deallocate(
	VkDeviceSize startingAddress, VkDeviceSize bufferSize, VkDeviceSize alignment
) noexcept
{
	m_allocator.Deallocate(
		static_cast<size_t>(startingAddress), static_cast<size_t>(bufferSize),
		static_cast<size_t>(alignment)
	);
}

// MemoryManager
MemoryManager::MemoryManager(
	VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize initialBudgetGPU,
	VkDeviceSize initialBudgetCPU
) : m_logicalDevice{ logicalDevice }, m_physicalDevice{ physicalDevice }, m_cpuAllocators{},
	m_gpuAllocators{}, m_availableGPUIndices{}, m_availableCPUIndices{}
{
	{
		// Try to allocate the CPU memory first, as it will be smaller and both types of memory might
		// share the same heap, so GPU is more likely to have less available memory than the
		// initialBudget.
		constexpr VkMemoryPropertyFlagBits cpuFlagBit = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		const VkDeviceSize availableCPUMemory         = GetAvailableMemoryOfType(cpuFlagBit);
		const VkDeviceSize cpuBudget                  = std::min(availableCPUMemory, initialBudgetCPU);
		std::optional<MemoryType> cpuMemType          = GetMemoryType(cpuBudget, cpuFlagBit);

		if (!cpuMemType) // This should alway succeed but still checking anywayr
			throw Exception("MemoryException", "Not Enough memory for allocation.");

		m_cpuAllocators.emplace_back(CreateMemory(cpuBudget, cpuMemType.value()), GetID(true));
	}

	{
		// Now try to allocate the gpu only memory.
		constexpr VkMemoryPropertyFlagBits gpuFlagBit = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		const VkDeviceSize availableGPUMemory         = GetAvailableMemoryOfType(gpuFlagBit);
		const VkDeviceSize gpuBudget                  = std::min(availableGPUMemory, initialBudgetGPU);
		std::optional<MemoryType> gpuMemType          = GetMemoryType(gpuBudget, gpuFlagBit);

		if (!gpuMemType) // This should alway succeed but still checking anyway.
			throw Exception("MemoryException", "Not Enough memory for allocation.");

		m_gpuAllocators.emplace_back(CreateMemory(gpuBudget, gpuMemType.value()), GetID(false));
	}
}

DeviceMemory2 MemoryManager::CreateMemory(VkDeviceSize size, MemoryType memoryType) const
{
	return DeviceMemory2{ m_logicalDevice, size, memoryType.index, memoryType.type };
}

std::optional<MemoryManager::MemoryType> MemoryManager::GetMemoryType(
	VkDeviceSize size, VkMemoryPropertyFlagBits memoryType
) const noexcept
{
	VkPhysicalDeviceMemoryBudgetPropertiesEXT memBudget
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT
	};
	VkPhysicalDeviceMemoryProperties2 memProp2
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
		.pNext = &memBudget
	};

	vkGetPhysicalDeviceMemoryProperties2(m_physicalDevice, &memProp2);

	// Check if the heap with the correct memory type has enough memory for allocation.
	const VkPhysicalDeviceMemoryProperties memProp = memProp2.memoryProperties;
	for (size_t index = 0u; index < memProp.memoryTypeCount; ++index)
	{
		if (const auto& memType = memProp.memoryTypes[index]; memType.propertyFlags & memoryType)
		{
			const std::uint32_t heapIndex = memType.heapIndex;
			const VkDeviceSize currentAvailableMemory =
				memBudget.heapBudget[heapIndex] - memBudget.heapUsage[heapIndex];

			if (currentAvailableMemory >= size)
				return MemoryType{ static_cast<std::uint32_t>(index), memoryType };
		}
	}

	return {};
}

VkDeviceSize MemoryManager::GetAvailableMemoryOfType(
	VkMemoryPropertyFlagBits memoryType
) const noexcept
{
	VkPhysicalDeviceMemoryBudgetPropertiesEXT memBudget
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT
	};
	VkPhysicalDeviceMemoryProperties2 memProp2
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
		.pNext = &memBudget
	};

	vkGetPhysicalDeviceMemoryProperties2(m_physicalDevice, &memProp2);

	// Check if the heap with the correct memory type has enough memory for allocation.
	const VkPhysicalDeviceMemoryProperties memProp = memProp2.memoryProperties;
	VkDeviceSize maxAvailableSize = 0u;

	for (size_t index = 0u; index < memProp.memoryTypeCount; ++index)
	{
		if (const auto& memType = memProp.memoryTypes[index]; memType.propertyFlags & memoryType)
		{
			const std::uint32_t heapIndex = memType.heapIndex;
			const VkDeviceSize currentAvailableMemory =
				memBudget.heapBudget[heapIndex] - memBudget.heapUsage[heapIndex];

			maxAvailableSize = std::max(maxAvailableSize, currentAvailableMemory);
		}
	}

	return maxAvailableSize;
}

VkDeviceSize MemoryManager::GetNewAllocationSize(VkMemoryPropertyFlagBits memoryType) const noexcept
{
	// Might add some algorithm here later.
	return memoryType & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ? 2_GB : 100_MB;
}

template<typename T>
MemoryManager::MemoryAllocation MemoryManager::Allocate(
	T resource, VkMemoryPropertyFlagBits memoryType
) {
	constexpr bool isBuffer                    = std::is_same_v<VkBuffer, T>;
	const VkMemoryRequirements memoryReq       = GetMemoryRequirements(m_logicalDevice, resource);
	const VkDeviceSize bufferSize              = memoryReq.size;
	const bool isCPUAccessible                 = memoryType & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	std::vector<VkAllocator>& allocators = isCPUAccessible ? m_cpuAllocators : m_gpuAllocators;

	// Look through the already existing allocators and try to allocate the buffer.
	for(size_t index = 0u; index < std::size(allocators); ++index)
	{
		VkAllocator& allocator = allocators[index];

		if (allocator.AvailableSize() >= bufferSize)
		{
			std::optional<VkDeviceSize> startingAddress{};
			if constexpr (isBuffer)
				startingAddress = allocator.AllocateBuffer(m_logicalDevice, memoryReq, resource);
			else
				startingAddress = allocator.AllocateImage(m_logicalDevice, memoryReq, resource);

			if (startingAddress)
			{
				const VkDeviceSize offset = startingAddress.value();
				std::uint8_t* cpuOffset
					= isCPUAccessible ? allocator.GetCPUStart() + offset : nullptr;

				MemoryAllocation allocation{
					.gpuOffset   = offset,
					.cpuOffset   = cpuOffset,
					.size        = bufferSize,
					.alignment   = memoryReq.alignment,
					.memoryID    = allocator.GetID()
				};

				return allocation;
			}
		}
	}

	{
		// If already available allocators were unable to allocate, then try to allocate new memory.
		VkDeviceSize newAllocationSize = std::max(bufferSize, GetNewAllocationSize(memoryType));

		std::optional<MemoryType> memType = GetMemoryType(newAllocationSize, memoryType);

		// If allocation is not possible, check if the buffer can be allocated on the available memory.
		if (!memType)
		{
			const VkDeviceSize availableMemorySize = GetAvailableMemoryOfType(memoryType);
			newAllocationSize = availableMemorySize;

			if (availableMemorySize >= bufferSize)
				memType = GetMemoryType(availableMemorySize, memoryType);
		}

		if (!memType)
			throw Exception("MemoryException", "Not Enough memory for allocation.");

		VkAllocator allocator{
			CreateMemory(newAllocationSize, memType.value()), GetID(isCPUAccessible)
		};

		std::optional<VkDeviceSize> startingAddress{};
		if constexpr (isBuffer)
			startingAddress = allocator.AllocateBuffer(m_logicalDevice, memoryReq, resource);
		else
			startingAddress = allocator.AllocateImage(m_logicalDevice, memoryReq, resource);

		if (startingAddress)
		{
			const VkDeviceSize offset = startingAddress.value();
			std::uint8_t* cpuOffset
				= isCPUAccessible ? allocator.GetCPUStart() + offset : nullptr;

			MemoryAllocation allocation{
				.gpuOffset   = offset,
				.cpuOffset   = cpuOffset,
				.size        = bufferSize,
				.alignment   = memoryReq.alignment,
				.memoryID    = allocator.GetID()
			};

			allocators.emplace_back(std::move(allocator));

			return allocation;
		}
		else
			throw Exception("MemoryException", "Not Enough memory for allocation.");
	}
}

MemoryManager::MemoryAllocation MemoryManager::AllocateBuffer(
	VkBuffer buffer, VkMemoryPropertyFlagBits memoryType
) {
	return Allocate(buffer, memoryType);
}

MemoryManager::MemoryAllocation MemoryManager::AllocateImage(
	VkImage image, VkMemoryPropertyFlagBits memoryType
) {
	return Allocate(image, memoryType);
}

void MemoryManager::Deallocate(
	const MemoryAllocation& allocation, VkMemoryPropertyFlagBits memoryType
) noexcept {
	const bool isCPUAccessible           = memoryType & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	std::vector<VkAllocator>& allocators = isCPUAccessible ? m_cpuAllocators : m_gpuAllocators;

	// Deallocate from the allocators.
	auto result = std::ranges::find_if(
		allocators,
		[id = allocation.memoryID](const VkAllocator& alloc) { return alloc.GetID() == id; }
	);

	if (result != std::end(allocators))
	{
		VkAllocator& allocator = *result;
		allocator.Deallocate(allocation.gpuOffset, allocation.size, allocation.alignment);

		// Check if the allocator is fully empty. If so deallocate the empty allocator.
		if (allocator.Size() == allocator.AvailableSize())
		{
			std::queue<std::uint16_t>& availableIndices
				= isCPUAccessible ? m_availableCPUIndices : m_availableGPUIndices;

			availableIndices.push(allocator.GetID());
			allocators.erase(result);
		}
	}
}

std::uint16_t MemoryManager::GetID(bool cpu) noexcept
{
	std::vector<VkAllocator>& allocators = cpu ? m_cpuAllocators : m_gpuAllocators;
	std::queue<std::uint16_t>& availableIndices = cpu ? m_availableCPUIndices : m_availableGPUIndices;

	if (std::empty(availableIndices))
		availableIndices.push(static_cast<std::uint16_t>(std::size(allocators)));

	const std::uint16_t ID = availableIndices.front();
	availableIndices.pop();

	return ID;
}
