#include <DeviceMemory.hpp>
#include <cstdint>
#include <ranges>
#include <concepts>

DeviceMemory::DeviceMemory(
	VkDevice device, VkDeviceSize size, std::uint32_t typeIndex, VkMemoryPropertyFlagBits type
) : m_device{ device }, m_memory{ VK_NULL_HANDLE }, m_size{ 0u }, m_mappedCPUMemory{ nullptr },
	m_memoryTypeIndex{ typeIndex }, m_memoryType{ type }
{
	Allocate(size);
}

DeviceMemory::~DeviceMemory() noexcept
{
	SelfDestruct();
}

void DeviceMemory::SelfDestruct() noexcept
{
	vkFreeMemory(m_device, m_memory, nullptr);
}

void DeviceMemory::Allocate(VkDeviceSize size)
{
	VkMemoryAllocateFlagsInfo flagsInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
	};

	VkMemoryAllocateInfo allocInfo{
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext           = &flagsInfo,
		.allocationSize  = size,
		.memoryTypeIndex = m_memoryTypeIndex
	};

	vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory);
	m_size = size;

	if (m_memoryType & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		vkMapMemory(
			m_device, m_memory, 0u, VK_WHOLE_SIZE, 0u, reinterpret_cast<void**>(&m_mappedCPUMemory)
		);
}
