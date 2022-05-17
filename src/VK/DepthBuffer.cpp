#include <DepthBuffer.hpp>
#include <VkHelperFunctions.hpp>
#include <VKThrowMacros.hpp>

DepthBuffer::DepthBuffer(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	std::vector<std::uint32_t> queueFamilyIndices
) :	m_depthImage(VK_NULL_HANDLE), m_depthImageView(VK_NULL_HANDLE),
	m_depthMemory(VK_NULL_HANDLE),
	m_deviceRef(logicalDevice), m_queueFamilyIndices(std::move(queueFamilyIndices)),
	m_memoryTypeIndex(0u) {

	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkImage tempImage = VK_NULL_HANDLE;

	CreateDepthImage(logicalDevice, &tempImage, 1u, 1u, DEPTHFORMAT);

	VkMemoryRequirements memoryReq = {};
	vkGetImageMemoryRequirements(logicalDevice, tempImage, &memoryReq);

	vkDestroyImage(logicalDevice, tempImage, nullptr);

	m_memoryTypeIndex = FindMemoryType(physicalDevice, memoryReq, properties);
}

DepthBuffer::~DepthBuffer() noexcept {
	CleanUp(m_deviceRef);
}

void DepthBuffer::CleanUp(VkDevice device) noexcept {
	vkDestroyImageView(device, m_depthImageView, nullptr);
	vkDestroyImage(device, m_depthImage, nullptr);
	vkFreeMemory(device, m_depthMemory, nullptr);
}

void DepthBuffer::CreateDepthImage(
	VkDevice device, VkImage* image,
	std::uint32_t width, std::uint32_t height,
	VkFormat depthFormat
) const {
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.depth = 1u;
	createInfo.extent.height = height;
	createInfo.extent.width = width;
	createInfo.mipLevels = 1u;
	createInfo.arrayLayers = 1u;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.flags = 0u;
	createInfo.format = depthFormat;

	ConfigureImageQueueAccess(m_queueFamilyIndices, createInfo);

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateImage(device, &createInfo, nullptr, image)
	);
}

void DepthBuffer::CreateDepthBuffer(
	VkDevice device,
	std::uint32_t width, std::uint32_t height
) {
	VkFormat depthFormat = DEPTHFORMAT;

	CreateDepthImage(device, &m_depthImage, width, height, depthFormat);

	AllocateMemory(device);

	VkResult result;
	VK_THROW_FAILED(result,
		vkBindImageMemory(
			device, m_depthImage, m_depthMemory, 0u
		)
	);

	CreateImageView(
		device, m_depthImage, &m_depthImageView,
		depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT
	);
}

void DepthBuffer::AllocateMemory(VkDevice device) {
	VkMemoryRequirements memoryReq = {};
	vkGetImageMemoryRequirements(device, m_depthImage, &memoryReq);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryReq.size;
	allocInfo.memoryTypeIndex = static_cast<std::uint32_t>(m_memoryTypeIndex);

	VkResult result;
	VK_THROW_FAILED(result,
		vkAllocateMemory(m_deviceRef, &allocInfo, nullptr, &m_depthMemory)
	);
}

VkFormat DepthBuffer::GetDepthFormat() const noexcept {
	return DEPTHFORMAT;
}

VkImageView DepthBuffer::GetDepthImageView() const noexcept {
	return m_depthImageView;
}
