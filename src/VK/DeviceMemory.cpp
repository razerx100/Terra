#include <DeviceMemory.hpp>
#include <cstdint>
#include <ranges>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

// Device memory
DeviceMemory::DeviceMemory(VkDevice logicalDevice, std::uint32_t memoryTypeIndex) noexcept
	: m_deviceRef(logicalDevice), m_bufferMemory(VK_NULL_HANDLE),
	m_memoryTypeIndex{ memoryTypeIndex }, m_totalSize{ 0u } {}

DeviceMemory::DeviceMemory(DeviceMemory&& deviceMemory) noexcept :
	m_deviceRef{ deviceMemory.m_deviceRef }, m_bufferMemory{ deviceMemory.m_bufferMemory },
	m_memoryTypeIndex{ deviceMemory.m_memoryTypeIndex },
	m_totalSize{ deviceMemory.m_totalSize } {

	deviceMemory.m_bufferMemory = VK_NULL_HANDLE;
}

DeviceMemory::~DeviceMemory() noexcept {
	vkFreeMemory(m_deviceRef, m_bufferMemory, nullptr);
}

DeviceMemory& DeviceMemory::operator=(DeviceMemory&& deviceMemory) noexcept {
	m_deviceRef = deviceMemory.m_deviceRef;
	m_bufferMemory = deviceMemory.m_bufferMemory;
	m_memoryTypeIndex = deviceMemory.m_memoryTypeIndex;
	m_totalSize = deviceMemory.m_totalSize;

	deviceMemory.m_bufferMemory = VK_NULL_HANDLE;

	return *this;
}

void DeviceMemory::AllocateMemory(VkDevice device) {
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = m_totalSize;
	allocInfo.memoryTypeIndex = m_memoryTypeIndex;

	VkResult result;
	VK_THROW_FAILED(result,
		vkAllocateMemory(device, &allocInfo, nullptr, &m_bufferMemory)
	);
}

VkDeviceMemory DeviceMemory::GetMemoryHandle() const noexcept {
	return m_bufferMemory;
}

std::uint32_t DeviceMemory::FindMemoryTypeIndex(
	VkPhysicalDevice physicalDevice,
	const VkMemoryRequirements& memoryReq, VkMemoryPropertyFlags propertiesToCheck
) noexcept {
	VkPhysicalDeviceMemoryProperties memoryProp{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProp);

	for (std::uint32_t index = 0u; index < memoryProp.memoryTypeCount; ++index) {
		// Check if the current memory type index is supported by the resource
		const bool indexAvailable = memoryReq.memoryTypeBits & (1u << index);
		// Check if the memory type with current index support the required properties flags
		const bool propertiesMatch =
			(memoryProp.memoryTypes[index].propertyFlags & propertiesToCheck)
			== propertiesToCheck;

		if (indexAvailable && propertiesMatch)
			return index;
	}

	return 0u;
}

VkDeviceSize DeviceMemory::ReserveSizeAndGetOffset(
	VkDeviceSize memorySize, VkDeviceSize alignment
) noexcept {
	const VkDeviceSize currentOffset = Align(m_totalSize, alignment);

	m_totalSize = currentOffset + memorySize;

	return currentOffset;
}

// Device memory manager
DeviceMemoryManager::DeviceMemoryManager(VkPhysicalDevice physicalDevice) noexcept
	: m_physicalDeviceRef{ physicalDevice } {}

void DeviceMemoryManager::AllocateMemory(VkDevice device) {
	for (auto& deviceMemory : m_memories | std::views::values)
		deviceMemory.AllocateMemory(device);
}

DeviceMemoryManager::MemoryData DeviceMemoryManager::ReserveSizeAndGetMemoryData(
	VkDevice device,
	const VkMemoryRequirements& memoryReq, VkMemoryPropertyFlags propertiesToCheck
) noexcept {
	const std::uint32_t memoryTypeIndex = DeviceMemory::FindMemoryTypeIndex(
		m_physicalDeviceRef, memoryReq, propertiesToCheck
	);

	VkDeviceSize offset = 0u;

	if (auto memory = m_memories.find(memoryTypeIndex); memory == std::end(m_memories)) {
		DeviceMemory newDeviceMemory{ device, memoryTypeIndex };
		offset = newDeviceMemory.ReserveSizeAndGetOffset(memoryReq.size, memoryReq.alignment);

		m_memories.emplace(memoryTypeIndex, std::move(newDeviceMemory));
	}
	else
		offset = memory->second.ReserveSizeAndGetOffset(memoryReq.size, memoryReq.alignment);

	return { offset, memoryTypeIndex };
}

VkDeviceMemory DeviceMemoryManager::GetMemoryHandle(std::uint32_t memoryIndex) const noexcept {
	if (auto memory = m_memories.find(memoryIndex); memory != std::end(m_memories))
		return memory->second.GetMemoryHandle();
	else
		return VK_NULL_HANDLE;
}
