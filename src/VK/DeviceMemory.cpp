#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>

DeviceMemory::DeviceMemory(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	bool uploadBuffer, BufferType type
) : m_deviceRef(logDevice), m_bufferMemory(VK_NULL_HANDLE),
	m_memoryTypeIndex(0u), m_alignment(0u) {

	VkBufferUsageFlags usage = 0u;
	VkMemoryPropertyFlags properties = 0u;

	if (uploadBuffer) {
		usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	}
	else {
		usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if (type == BufferType::Vertex)
			usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		else if (type == BufferType::Index)
			usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		else if (type == BufferType::UniformAndStorage)
			usage |=
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = 1u;
	bufferInfo.usage = usage;

	ConfigureBufferQueueAccess(queueFamilyIndices, bufferInfo);

	VkBuffer tempBuffer = nullptr;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateBuffer(logDevice, &bufferInfo, nullptr, &tempBuffer)
	);

	VkMemoryRequirements memoryReq = {};
	vkGetBufferMemoryRequirements(logDevice, tempBuffer, &memoryReq);

	m_alignment = memoryReq.alignment;

	VkPhysicalDeviceMemoryProperties memoryProp = {};
	vkGetPhysicalDeviceMemoryProperties(phyDevice, &memoryProp);

	for(size_t index = 0u; index < memoryProp.memoryTypeCount; ++index)
		if ((memoryReq.memoryTypeBits & (1u << index))
			&& (memoryProp.memoryTypes[index].propertyFlags & properties) == properties) {
			m_memoryTypeIndex = index;
			break;
		}

	vkDestroyBuffer(logDevice, tempBuffer, nullptr);
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

void ConfigureBufferQueueAccess(
	const std::vector<std::uint32_t>& queueFamilyIndice,
	VkBufferCreateInfo& bufferInfo
) {
	if (queueFamilyIndice.size() > 1u) {
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bufferInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(queueFamilyIndice.size());
		bufferInfo.pQueueFamilyIndices = queueFamilyIndice.data();
	}
	else
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
}
