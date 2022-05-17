#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

DeviceMemory::DeviceMemory(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	bool uploadBuffer, BufferType type
) : m_deviceRef(logicalDevice), m_bufferMemory(VK_NULL_HANDLE),
	m_memoryTypeIndex(0u), m_alignment(0u) {

	VkMemoryRequirements memoryReq = {};
	VkMemoryPropertyFlags properties = 0u;

	if (uploadBuffer) {
		properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		UploadBuffer buffer = UploadBuffer(logicalDevice);

		buffer.CreateBuffer(logicalDevice, 1u);

		vkGetBufferMemoryRequirements(logicalDevice, buffer.GetBuffer(), &memoryReq);
	}
	else {
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if(type == BufferType::Image) {
			ImageBuffer image = ImageBuffer(logicalDevice);

			image.CreateImage(
				logicalDevice,
				1u, 1u, VK_FORMAT_R8G8B8A8_SRGB,
				queueFamilyIndices
			);

			vkGetImageMemoryRequirements(logicalDevice, image.GetImage(), &memoryReq);
		}
		else {
			GpuBuffer buffer = GpuBuffer(logicalDevice);

			buffer.CreateBuffer(
				logicalDevice,
				1u, queueFamilyIndices, type
			);

			vkGetBufferMemoryRequirements(logicalDevice, buffer.GetBuffer(), &memoryReq);
		}
	}

	m_alignment = memoryReq.alignment;

	m_memoryTypeIndex = FindMemoryType(physicalDevice, memoryReq, properties);
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
