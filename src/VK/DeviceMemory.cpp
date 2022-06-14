#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

DeviceMemory::DeviceMemory(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	const VkMemoryRequirements& memoryRequirements, bool uploadBuffer
) : m_deviceRef(logicalDevice), m_bufferMemory(VK_NULL_HANDLE), m_memoryTypeIndex(0u) {

	VkMemoryPropertyFlags properties =
		uploadBuffer ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_memoryTypeIndex = FindMemoryType(physicalDevice, memoryRequirements, properties);
}

DeviceMemory::~DeviceMemory() noexcept {
	vkFreeMemory(m_deviceRef, m_bufferMemory, nullptr);
}

void DeviceMemory::AllocateMemory(size_t memorySize) {
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memorySize;
	allocInfo.memoryTypeIndex = static_cast<std::uint32_t>(m_memoryTypeIndex);

	VkResult result;
	VK_THROW_FAILED(result,
		vkAllocateMemory(m_deviceRef, &allocInfo, nullptr, &m_bufferMemory)
	);
}

VkDeviceMemory DeviceMemory::GetMemoryHandle() const noexcept {
	return m_bufferMemory;
}
