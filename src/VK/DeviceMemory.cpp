#include <DeviceMemory.hpp>
#include <cstdint>
#include <ranges>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

// Device memory
DeviceMemory::DeviceMemory(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlagBits memoryType
) noexcept
	: m_deviceRef{ logicalDevice }, m_bufferMemory{ VK_NULL_HANDLE },
	m_memoryTypeIndex{ FindMemoryTypeIndex(physicalDevice, memoryType) }, m_totalSize{ 0u },
	m_memoryType{ memoryType }, m_mappedCPUPtr{ nullptr } {}

DeviceMemory::DeviceMemory(DeviceMemory&& deviceMemory) noexcept :
	m_deviceRef{ deviceMemory.m_deviceRef }, m_bufferMemory{ deviceMemory.m_bufferMemory },
	m_memoryTypeIndex{ deviceMemory.m_memoryTypeIndex },
	m_totalSize{ deviceMemory.m_totalSize }, m_memoryType{ deviceMemory.m_memoryType },
	m_mappedCPUPtr{ deviceMemory.m_mappedCPUPtr } {

	deviceMemory.m_bufferMemory = VK_NULL_HANDLE;
	deviceMemory.m_mappedCPUPtr = nullptr;
}

DeviceMemory::~DeviceMemory() noexcept {
	vkFreeMemory(m_deviceRef, m_bufferMemory, nullptr);
}

DeviceMemory& DeviceMemory::operator=(DeviceMemory&& deviceMemory) noexcept {
	m_deviceRef = deviceMemory.m_deviceRef;
	m_bufferMemory = deviceMemory.m_bufferMemory;
	m_memoryTypeIndex = deviceMemory.m_memoryTypeIndex;
	m_totalSize = deviceMemory.m_totalSize;
	m_memoryType = deviceMemory.m_memoryType;
	m_mappedCPUPtr = deviceMemory.m_mappedCPUPtr;

	deviceMemory.m_bufferMemory = VK_NULL_HANDLE;
	deviceMemory.m_mappedCPUPtr = nullptr;

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
	VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags propertiesToCheck
) noexcept {
	VkPhysicalDeviceMemoryProperties memoryProp{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProp);

	for (std::uint32_t index = 0u; index < memoryProp.memoryTypeCount; ++index) {
		// Check if the memory type with current index support the required properties flags
		const bool propertiesMatch =
			(memoryProp.memoryTypes[index].propertyFlags & propertiesToCheck)
			== propertiesToCheck;

		if (propertiesMatch)
			return index;
	}

	return 0u;
}

bool DeviceMemory::CheckMemoryType(const VkMemoryRequirements& memoryReq) const noexcept {
	return memoryReq.memoryTypeBits & (1u << m_memoryTypeIndex);
}

VkDeviceSize DeviceMemory::ReserveSizeAndGetOffset(
	const VkMemoryRequirements& memoryReq
) noexcept {
	const VkDeviceSize currentOffset = Align(m_totalSize, memoryReq.alignment);

	m_totalSize = currentOffset + memoryReq.size;

	return currentOffset;
}

void DeviceMemory::MapMemoryToCPU(VkDevice device) {
	if (m_memoryType != VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		VK_GENERIC_THROW("Memory isn't CPU accessable.");

	VkResult result{};
	VK_THROW_FAILED(result,
		vkMapMemory(
			device, m_bufferMemory, 0u, VK_WHOLE_SIZE, 0u,
			reinterpret_cast<void**>(&m_mappedCPUPtr)
		)
	);
}

std::uint8_t* DeviceMemory::GetMappedCPUPtr() const noexcept {
	return m_mappedCPUPtr;
}
