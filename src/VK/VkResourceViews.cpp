#include <VkResourceViews.hpp>
#include <VKThrowMacros.hpp>

// Vk Resource view
VkResourceView::VkResourceView(VkDevice device) noexcept
	: m_resource{ device }, m_memoryOffset{ 0u } {}

VkResourceView::VkResourceView(VkResourceView&& resourceView) noexcept
	: m_resource{ std::move(resourceView.m_resource) },
	m_memoryOffset { resourceView.m_memoryOffset } {}

VkResourceView& VkResourceView::operator=(VkResourceView&& resourceView) noexcept {
	m_resource = std::move(resourceView.m_resource);
	m_memoryOffset = resourceView.m_memoryOffset;

	return *this;
}

void VkResourceView::CreateResource(
	VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
	std::vector<std::uint32_t> queueFamilyIndices
) {
	m_resource.CreateResource(device, bufferSize, usageFlags, queueFamilyIndices);
}

void VkResourceView::BindResourceToMemory(VkDevice device, VkDeviceMemory memory) {
	VkResult result{};
	VK_THROW_FAILED(result,
		vkBindBufferMemory(
			device, m_resource.GetResource(), memory, m_memoryOffset
		)
	);
}

void VkResourceView::SetMemoryOffset(VkDeviceSize offset) noexcept {
	m_memoryOffset = offset;
}

VkBuffer VkResourceView::GetResource() const noexcept {
	return m_resource.GetResource();
}

VkDeviceSize VkResourceView::GetMemoryOffset() const noexcept {
	return m_memoryOffset;
}

// Vk Image Resource View
VkImageResourceView::VkImageResourceView(VkDevice device) noexcept
	: m_deviceRef{ device }, m_resource{ device }, m_imageView{ VK_NULL_HANDLE },
	m_imageWidth{ 0u }, m_imageHeight{ 0u }, m_imageFormat{ VK_FORMAT_UNDEFINED },
	m_memoryOffset{ 0u } {}

VkImageResourceView::~VkImageResourceView() noexcept {
	vkDestroyImageView(m_deviceRef, m_imageView, nullptr);
}

VkImageResourceView::VkImageResourceView(VkImageResourceView&& imageResourceView) noexcept
	: m_deviceRef{ imageResourceView.m_deviceRef },
	m_resource{ std::move(imageResourceView.m_resource) },
	m_imageView{ imageResourceView.m_imageView }, m_imageWidth{ imageResourceView.m_imageWidth },
	m_imageHeight{ imageResourceView.m_imageHeight },
	m_imageFormat{ imageResourceView.m_imageFormat },
	m_memoryOffset{ imageResourceView.m_memoryOffset } {

	imageResourceView.m_imageView = VK_NULL_HANDLE;
}

VkImageResourceView& VkImageResourceView::operator=(
	VkImageResourceView&& imageResourceView
) noexcept {
	m_deviceRef = imageResourceView.m_deviceRef;
	m_resource = std::move(imageResourceView.m_resource);
	m_imageView = imageResourceView.m_imageView;
	m_imageWidth = imageResourceView.m_imageWidth;
	m_imageHeight = imageResourceView.m_imageHeight;
	m_imageFormat = imageResourceView.m_imageFormat;
	m_memoryOffset = imageResourceView.m_memoryOffset;

	imageResourceView.m_imageView = VK_NULL_HANDLE;

	return *this;
}

void VkImageResourceView::CreateResource(
	VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
	VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
) {
	m_resource.CreateResource(
		device, width, height, imageFormat, usageFlags, queueFamilyIndices
	);

	m_imageWidth = width;
	m_imageHeight = height;
	m_imageFormat = imageFormat;
}

void VkImageResourceView::SetMemoryOffset(VkDeviceSize offset) noexcept {
	m_memoryOffset = offset;
}

void VkImageResourceView::BindResourceToMemory(VkDevice device, VkDeviceMemory memory) {
	VkResult result{};
	VK_THROW_FAILED(result,
		vkBindImageMemory(device, m_resource.GetResource(), memory, m_memoryOffset)
	);
}

void VkImageResourceView::_createImageView(
	VkDevice device, VkImage image, VkImageView* imageView,
	VkFormat imageFormat, VkImageAspectFlagBits aspectBit
) {
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = imageFormat;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = aspectBit;
	createInfo.subresourceRange.baseMipLevel = 0u;
	createInfo.subresourceRange.levelCount = 1u;
	createInfo.subresourceRange.baseArrayLayer = 0u;
	createInfo.subresourceRange.layerCount = 1u;

	VkResult result{};
	VK_THROW_FAILED(result,
		vkCreateImageView(device, &createInfo, nullptr, imageView)
	);
}

void VkImageResourceView::CreateImageView(
	VkDevice device, VkImageAspectFlagBits aspectBit
) {
	_createImageView(device, m_resource.GetResource(), &m_imageView, m_imageFormat, aspectBit);
}

void VkImageResourceView::CleanUpImageResourceView() noexcept {
	vkDestroyImageView(m_deviceRef, m_imageView, nullptr);
	m_resource.CleanUpResource();
}

void VkImageResourceView::RecordImageCopy(
	VkCommandBuffer copyCmdBuffer, VkBuffer uploadBuffer
) noexcept {
	VkImageSubresourceLayers imageLayer{};
	imageLayer.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageLayer.mipLevel = 0u;
	imageLayer.baseArrayLayer = 0u;
	imageLayer.layerCount = 1u;

	VkOffset3D imageOffset{};
	imageOffset.x = 0u;
	imageOffset.y = 0u;
	imageOffset.z = 0u;

	VkExtent3D imageExtent{};
	imageExtent.depth = 1u;
	imageExtent.height = m_imageHeight;
	imageExtent.width = m_imageWidth;

	VkBufferImageCopy copyRegion{};
	copyRegion.bufferOffset = 0u;
	copyRegion.bufferRowLength = 0u;
	copyRegion.bufferImageHeight = 0u;
	copyRegion.imageSubresource = imageLayer;
	copyRegion.imageOffset = imageOffset;
	copyRegion.imageExtent = imageExtent;

	TransitionImageLayout(copyCmdBuffer, false);

	vkCmdCopyBufferToImage(
		copyCmdBuffer, uploadBuffer,
		m_resource.GetResource(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1u, &copyRegion
	);
}

void VkImageResourceView::TransitionImageLayout(
	VkCommandBuffer cmdBuffer, bool shaderStage
) noexcept {
	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0u;
	subresourceRange.levelCount = 1u;
	subresourceRange.baseArrayLayer = 0u;
	subresourceRange.layerCount = 1u;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_resource.GetResource();
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

VkImageView VkImageResourceView::GetImageView() const noexcept {
	return m_imageView;
}

VkImage VkImageResourceView::GetResource() const noexcept {
	return m_resource.GetResource();
}
