#include <DeviceMemory.hpp>
#include <cstdint>
#include <ranges>
#include <concepts>

template<std::integral Integer>
[[nodiscard]]
static constexpr Integer Align(Integer address, Integer alignment) noexcept {
	const auto _address = static_cast<size_t>(address);
	const auto _alignment = static_cast<size_t>(alignment);
	return static_cast<Integer>((_address + (_alignment - 1u)) & ~(_alignment - 1u));
}

static std::uint32_t FindMemoryTypeIndex(
	VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags propertiesToCheck
) noexcept
{
	VkPhysicalDeviceMemoryProperties memoryProp{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProp);

	for (std::uint32_t index = 0u; index < memoryProp.memoryTypeCount; ++index)
	{
		// Check if the memory type with current index support the required properties flags
		const bool propertiesMatch =
			(memoryProp.memoryTypes[index].propertyFlags & propertiesToCheck)
			== propertiesToCheck;

		if (propertiesMatch)
			return index;
	}

	return 0u;
}


// Device memory
DeviceMemory::DeviceMemory(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlagBits memoryType
) : m_deviceRef{ logicalDevice }, m_bufferMemory{ VK_NULL_HANDLE },
	m_memoryTypeIndex{ FindMemoryTypeIndex(physicalDevice, memoryType) },
	m_totalSize{ 0u }, m_memoryType{ memoryType }, m_mappedCPUPtr{ nullptr } {}

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

	vkAllocateMemory(device, &allocInfo, nullptr, &m_bufferMemory);
}

VkDeviceMemory DeviceMemory::GetMemoryHandle() const noexcept {
	return m_bufferMemory;
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
	assert(
		m_memoryType & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT && "Memory isn't CPU accessable."
	);

	vkMapMemory(
		device, m_bufferMemory, 0u, VK_WHOLE_SIZE, 0u,
		reinterpret_cast<void**>(&m_mappedCPUPtr)
	);
}

std::uint8_t* DeviceMemory::GetMappedCPUPtr() const noexcept {
	return m_mappedCPUPtr;
}

// DeviceMemory2
DeviceMemory2::DeviceMemory2(
	VkDevice device, VkDeviceSize size, std::uint32_t typeIndex, VkMemoryPropertyFlagBits type
) : m_device{ device }, m_memory{ VK_NULL_HANDLE }, m_size{ 0u }, m_mappedCPUMemory{ nullptr }
	, m_memoryTypeIndex{ typeIndex }, m_memoryType{ type }
{
	Allocate(size);
}

DeviceMemory2::~DeviceMemory2() noexcept
{
	vkFreeMemory(m_device, m_memory, nullptr);
}

void DeviceMemory2::Allocate(VkDeviceSize size)
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
