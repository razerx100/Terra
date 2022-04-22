#include <VkBuffers.hpp>
#include <VKThrowMacros.hpp>
#include <ObjectCreationFunctions.hpp>

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
) noexcept
	: m_deviceRef(device),
	m_image(VK_NULL_HANDLE), m_imageView(VK_NULL_HANDLE) {}

ImageBuffer::~ImageBuffer() noexcept {
	vkDestroyImageView(m_deviceRef, m_imageView, nullptr);
	vkDestroyImage(m_deviceRef, m_image, nullptr);
}

void ImageBuffer::CreateImage(
	VkDevice device,
	std::uint32_t width, std::uint32_t height,
	VkFormat imageFormat,
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
	createInfo.format = imageFormat;

	ConfigureImageQueueAccess(queueFamilyIndices, createInfo);

	VkResult result;

	VK_THROW_FAILED(result,
		vkCreateImage(device, &createInfo, nullptr, &m_image)
	);
}

VkImageView ImageBuffer::GetImageView() const noexcept {
	return m_imageView;
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

void ImageBuffer::CopyToImage(
	VkCommandBuffer copyCmdBuffer, VkBuffer uploadBuffer,
	std::uint32_t width, std::uint32_t height
) noexcept {
	VkImageSubresourceLayers imageLayer = {};
	imageLayer.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageLayer.mipLevel = 0u;
	imageLayer.baseArrayLayer = 0u;
	imageLayer.layerCount = 1u;

	VkOffset3D imageOffset = {};
	imageOffset.x = 0u;
	imageOffset.y = 0u;
	imageOffset.z = 0u;

	VkExtent3D imageExtent = {};
	imageExtent.depth = 1u;
	imageExtent.height = height;
	imageExtent.width = width;

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0u;
	copyRegion.bufferRowLength = 0u;
	copyRegion.bufferImageHeight = 0u;
	copyRegion.imageSubresource = imageLayer;
	copyRegion.imageOffset = imageOffset;
	copyRegion.imageExtent = imageExtent;

	TransitionImageLayout(copyCmdBuffer, false);

	vkCmdCopyBufferToImage(
		copyCmdBuffer, uploadBuffer,
		m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1u, &copyRegion
	);
}

void ImageBuffer::TransitionImageLayout(
	VkCommandBuffer cmdBuffer, bool shaderStage
) noexcept {
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0u;
	subresourceRange.levelCount = 1u;
	subresourceRange.baseArrayLayer = 0u;
	subresourceRange.layerCount = 1u;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange = subresourceRange;

	VkPipelineStageFlags sourceStage = 0u;
	VkPipelineStageFlags destinationStage = 0u;

	if (shaderStage) {
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = 0u;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	vkCmdPipelineBarrier(
		cmdBuffer,
		sourceStage, destinationStage,
		0u,
		0u, nullptr,
		0u, nullptr,
		1u, &barrier
	);
}

void ImageBuffer::BindImageToMemory(
	VkDevice device, VkDeviceMemory memory,
	VkDeviceSize offset
) {
	VkResult result;
	VK_THROW_FAILED(result,
		vkBindImageMemory(
			device, m_image,
			memory, offset
		)
	);
}

void ImageBuffer::CreateImageView(
	VkDevice device, VkFormat format
) noexcept {
	::CreateImageView(
		device, m_image,
		&m_imageView, format
	);
}
