#include <VkBuffers.hpp>
#include <VKThrowMacros.hpp>

// Base Buffer
BaseBuffer::BaseBuffer(
	VkDevice device
) noexcept : m_buffer(VK_NULL_HANDLE), m_deviceRef(device) {}

BaseBuffer::~BaseBuffer() noexcept {
	vkDestroyBuffer(m_deviceRef, m_buffer, nullptr);
}

VkBuffer BaseBuffer::GetBuffer() const noexcept {
	return m_buffer;
}

void BaseBuffer::ConfigureBufferQueueAccess(
	const std::vector<std::uint32_t>& queueFamilyIndices,
	VkBufferCreateInfo& bufferInfo
) noexcept {
	if (queueFamilyIndices.size() > 1u) {
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bufferInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(queueFamilyIndices.size());
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}
	else
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
}

// Upload Buffer

UploadBuffer::UploadBuffer(
	VkDevice device
) noexcept : BaseBuffer(device) {}

void UploadBuffer::CreateBuffer(
	VkDevice device, size_t bufferSize
) {
	VkBufferCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = static_cast<VkDeviceSize>(bufferSize);

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateBuffer(device, &createInfo, nullptr, &m_buffer)
	);
}

// Gpu Buffer

GpuBuffer::GpuBuffer(
	VkDevice device
) noexcept : BaseBuffer(device) {}

void GpuBuffer::CreateBuffer(
	VkDevice device, size_t bufferSize,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	BufferType type
) {
	VkBufferCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	createInfo.size = static_cast<VkDeviceSize>(bufferSize);

	if (type == BufferType::Vertex)
		createInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	else if (type == BufferType::Index)
		createInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	else if (type == BufferType::UniformAndStorage)
		createInfo.usage |=
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	ConfigureBufferQueueAccess(queueFamilyIndices, createInfo);

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateBuffer(device, &createInfo, nullptr, &m_buffer)
	);
}

// Image Buffer

ImageBuffer::ImageBuffer(
	VkDevice device
) noexcept : m_deviceRef(device), m_image(VK_NULL_HANDLE) {}

ImageBuffer::~ImageBuffer() noexcept {
	vkDestroyImage(m_deviceRef, m_image, nullptr);
}

void ImageBuffer::CreateImage(
	VkDevice device,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t pixelSizeInBytes,
	const std::vector<std::uint32_t>& queueFamilyIndices
) {
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
	createInfo.usage =
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.flags = 0u;

	if (pixelSizeInBytes == 4u)
		createInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	else if (pixelSizeInBytes == 16u)
		createInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	ConfigureImageQueueAccess(queueFamilyIndices, createInfo);

	VkResult result;

	VK_THROW_FAILED(result,
		vkCreateImage(device, &createInfo, nullptr, &m_image)
	);
}

VkImage ImageBuffer::GetImage() const noexcept {
	return m_image;
}

void ImageBuffer::ConfigureImageQueueAccess(
	const std::vector<std::uint32_t>& queueFamilyIndices,
	VkImageCreateInfo& imageInfo
) noexcept {
	if (queueFamilyIndices.size() > 1u) {
		imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		imageInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(queueFamilyIndices.size());
		imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}
	else
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
}
