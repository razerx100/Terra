#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>

DeviceMemory::DeviceMemory(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	bool uploadBuffer, BufferType type
) : m_deviceRef(logDevice), m_bufferMemory(VK_NULL_HANDLE),
	m_memoryTypeIndex(0u), m_alignment(0u) {

	VkMemoryRequirements memoryReq = {};
	VkMemoryPropertyFlags properties = 0u;

	if (uploadBuffer) {
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		UploadBuffer buffer = UploadBuffer(logDevice);

		buffer.CreateBuffer(logDevice, 1u);

		vkGetBufferMemoryRequirements(logDevice, buffer.GetBuffer(), &memoryReq);
	}
	else {
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		GpuBuffer buffer = GpuBuffer(logDevice);

		buffer.CreateBuffer(
			logDevice,
			1u, queueFamilyIndices, type
		);

		vkGetBufferMemoryRequirements(logDevice, buffer.GetBuffer(), &memoryReq);
	}

	m_alignment = memoryReq.alignment;

	VkPhysicalDeviceMemoryProperties memoryProp = {};
	vkGetPhysicalDeviceMemoryProperties(phyDevice, &memoryProp);

	for(size_t index = 0u; index < memoryProp.memoryTypeCount; ++index)
		if ((memoryReq.memoryTypeBits & (1u << index))
			&& (memoryProp.memoryTypes[index].propertyFlags & properties) == properties) {
			m_memoryTypeIndex = index;
			break;
		}
}

DeviceMemory::~DeviceMemory() noexcept {
	vkFreeMemory(m_deviceRef, m_bufferMemory, nullptr);
}

void DeviceMemory::AllocateMemory(
	size_t memorySize
) {
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

size_t DeviceMemory::GetAlignment() const noexcept {
	return m_alignment;
}
