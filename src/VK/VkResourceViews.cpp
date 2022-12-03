#include <VkResourceViews.hpp>
#include <VkResourceBarriers.hpp>

#include <Terra.hpp>

// Vk Resource view
VkResourceView::VkResourceView(VkDevice device) noexcept
	: m_resource{ device }, m_memoryOffsetStart{ 0u }, m_bufferSize{ 0u },
	m_resourceType{ MemoryType::none }, m_subAllocationSize{ 0u } {}

VkResourceView::VkResourceView(VkResourceView&& resourceView) noexcept
	: m_resource{ std::move(resourceView.m_resource) },
	m_memoryOffsetStart{ resourceView.m_memoryOffsetStart },
	m_bufferSize{ resourceView.m_bufferSize }, m_resourceType{ resourceView.m_resourceType },
	m_subAllocationSize{ resourceView.m_subAllocationSize } {}

VkResourceView& VkResourceView::operator=(VkResourceView&& resourceView) noexcept {
	m_resource = std::move(resourceView.m_resource);
	m_memoryOffsetStart = resourceView.m_memoryOffsetStart;
	m_bufferSize = resourceView.m_bufferSize;
	m_resourceType = resourceView.m_resourceType;
	m_subAllocationSize = resourceView.m_subAllocationSize;

	return *this;
}

void VkResourceView::CreateResource(
	VkDevice device, VkDeviceSize bufferSize, std::uint32_t subAllocationCount,
	VkBufferUsageFlags usageFlags, std::vector<std::uint32_t> queueFamilyIndices
) {
	m_subAllocationSize = bufferSize;
	m_bufferSize = bufferSize * subAllocationCount;
	m_resource.CreateResource(device, m_bufferSize, usageFlags, queueFamilyIndices);
}

void VkResourceView::BindResourceToMemory(VkDevice device) {
	VkDeviceMemory resourceMemoryStart = VK_NULL_HANDLE;

	if (m_resourceType == MemoryType::upload)
		resourceMemoryStart = Terra::Resources::uploadMemory->GetMemoryHandle();
	else if (m_resourceType == MemoryType::cpuWrite)
		resourceMemoryStart = Terra::Resources::cpuWriteMemory->GetMemoryHandle();
	else if (m_resourceType == MemoryType::gpuOnly)
		resourceMemoryStart = Terra::Resources::gpuOnlyMemory->GetMemoryHandle();

	vkBindBufferMemory(
		device, m_resource.GetResource(), resourceMemoryStart, m_memoryOffsetStart
	);
}

void VkResourceView::SetMemoryOffsetAndType(VkDevice device, MemoryType type) noexcept {
	const auto memoryReq = GetMemoryRequirements(device);
	m_resourceType = type;

	if (type == MemoryType::upload)
		m_memoryOffsetStart = Terra::Resources::uploadMemory->ReserveSizeAndGetOffset(
			memoryReq
		);
	else if (type == MemoryType::cpuWrite)
		m_memoryOffsetStart = Terra::Resources::cpuWriteMemory->ReserveSizeAndGetOffset(
			memoryReq
		);
	else if (type == MemoryType::gpuOnly)
		m_memoryOffsetStart = Terra::Resources::gpuOnlyMemory->ReserveSizeAndGetOffset(
			memoryReq
		);
}

void VkResourceView::SetMemoryOffsetAndType(VkDeviceSize offset, MemoryType type) noexcept {
	m_memoryOffsetStart = offset;
	m_resourceType = type;
}

void VkResourceView::CleanUpResource() noexcept {
	m_resource.CleanUpResource();
}

void VkResourceView::RecordCopy(
	VkCommandBuffer copyCmdBuffer, VkBuffer uploadBuffer
) noexcept {
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0u;
	copyRegion.dstOffset = 0u;
	copyRegion.size = m_bufferSize;

	vkCmdCopyBuffer(
		copyCmdBuffer, uploadBuffer, m_resource.GetResource(), 1u, &copyRegion
	);
}

void VkResourceView::ReleaseOwnerShip(
	VkCommandBuffer copyCmdBuffer, std::uint32_t oldOwnerQueueIndex,
	std::uint32_t newOwnerQueueIndex
) noexcept {
	VkBufferBarrier().AddBarrier(
		m_resource.GetResource(), m_bufferSize, 0u,
		oldOwnerQueueIndex, newOwnerQueueIndex,
		VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_NONE,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE
	).RecordBarriers(copyCmdBuffer);
}

void VkResourceView::AcquireOwnership(
	VkCommandBuffer cmdBuffer, std::uint32_t oldOwnerQueueIndex,
	std::uint32_t newOwnerQueueIndex, VkAccessFlagBits2 destinationAccess,
	VkPipelineStageFlagBits2 destinationStage
) noexcept {
	VkBufferBarrier().AddBarrier(
		m_resource.GetResource(), m_bufferSize, 0u,
		oldOwnerQueueIndex, newOwnerQueueIndex,
		VK_ACCESS_2_NONE, destinationAccess,
		VK_PIPELINE_STAGE_2_NONE, destinationStage
	).RecordBarriers(cmdBuffer);
}

VkBuffer VkResourceView::GetResource() const noexcept {
	return m_resource.GetResource();
}

VkDeviceSize VkResourceView::GetSubAllocationOffset(VkDeviceSize index) const noexcept {
	return m_subAllocationSize * index;
}

VkDeviceSize VkResourceView::GetMemoryOffset(VkDeviceSize index) const noexcept {
	return m_memoryOffsetStart + GetSubAllocationOffset(index);
}

VkMemoryRequirements VkResourceView::GetMemoryRequirements(VkDevice device) const noexcept {
	return m_resource.GetMemoryRequirements(device);
}

VkDeviceSize VkResourceView::GetFirstMemoryOffset() const noexcept {
	return GetMemoryOffset(0u);
}

VkDeviceSize VkResourceView::GetFirstSubAllocationOffset() const noexcept {
	return GetSubAllocationOffset(0u);
}

VkDeviceSize VkResourceView::GetSubAllocationSize() const noexcept {
	return m_subAllocationSize;
}

// Vk Image Resource View
VkImageResourceView::VkImageResourceView(VkDevice device) noexcept
	: m_deviceRef{ device }, m_resource{ device }, m_imageView{ VK_NULL_HANDLE },
	m_imageWidth{ 0u }, m_imageHeight{ 0u }, m_imageFormat{ VK_FORMAT_UNDEFINED },
	m_memoryOffset{ 0u }, m_resourceType{ MemoryType::none } {}

VkImageResourceView::~VkImageResourceView() noexcept {
	vkDestroyImageView(m_deviceRef, m_imageView, nullptr);
}

VkImageResourceView::VkImageResourceView(VkImageResourceView&& imageResourceView) noexcept
	: m_deviceRef{ imageResourceView.m_deviceRef },
	m_resource{ std::move(imageResourceView.m_resource) },
	m_imageView{ imageResourceView.m_imageView }, m_imageWidth{ imageResourceView.m_imageWidth },
	m_imageHeight{ imageResourceView.m_imageHeight },
	m_imageFormat{ imageResourceView.m_imageFormat },
	m_memoryOffset{ imageResourceView.m_memoryOffset },
	m_resourceType{ imageResourceView.m_resourceType } {

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
	m_resourceType = imageResourceView.m_resourceType;

	imageResourceView.m_imageView = VK_NULL_HANDLE;

	return *this;
}

void VkImageResourceView::CreateResource(
	VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
	VkImageUsageFlags usageFlags, std::vector<std::uint32_t> queueFamilyIndices
) {
	m_resource.CreateResource(
		device, width, height, imageFormat, usageFlags, std::move(queueFamilyIndices)
	);

	m_imageWidth = width;
	m_imageHeight = height;
	m_imageFormat = imageFormat;
}

void VkImageResourceView::SetMemoryOffsetAndType(VkDevice device, MemoryType type) noexcept {
	const auto memoryReq = GetMemoryRequirements(device);
	m_resourceType = type;

	if (type == MemoryType::upload)
		m_memoryOffset = Terra::Resources::uploadMemory->ReserveSizeAndGetOffset(memoryReq);
	else if (type == MemoryType::cpuWrite)
		m_memoryOffset = Terra::Resources::cpuWriteMemory->ReserveSizeAndGetOffset(memoryReq);
	else if (type == MemoryType::gpuOnly)
		m_memoryOffset = Terra::Resources::gpuOnlyMemory->ReserveSizeAndGetOffset(memoryReq);
}

void VkImageResourceView::SetMemoryOffsetAndType(VkDeviceSize offset, MemoryType type) noexcept {
	m_memoryOffset = offset;
	m_resourceType = type;
}

void VkImageResourceView::BindResourceToMemory(VkDevice device) {
	VkDeviceMemory resourceMemoryStart = VK_NULL_HANDLE;

	if (m_resourceType == MemoryType::upload)
		resourceMemoryStart = Terra::Resources::uploadMemory->GetMemoryHandle();
	else if (m_resourceType == MemoryType::cpuWrite)
		resourceMemoryStart = Terra::Resources::cpuWriteMemory->GetMemoryHandle();
	else if (m_resourceType == MemoryType::gpuOnly)
		resourceMemoryStart = Terra::Resources::gpuOnlyMemory->GetMemoryHandle();

	vkBindImageMemory(
		device, m_resource.GetResource(), resourceMemoryStart, m_memoryOffset
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

	vkCreateImageView(device, &createInfo, nullptr, imageView);
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

void VkImageResourceView::RecordCopy(
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

	VkImageBarrier().AddLayoutBarrier(
		m_resource.GetResource(), VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_2_NONE, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_TRANSFER_BIT
	).RecordBarriers(copyCmdBuffer);

	vkCmdCopyBufferToImage(
		copyCmdBuffer, uploadBuffer,
		m_resource.GetResource(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1u, &copyRegion
	);
}

void VkImageResourceView::ReleaseOwnerShip(
	VkCommandBuffer copyCmdBuffer, std::uint32_t oldOwnerQueueIndex,
	std::uint32_t newOwnerQueueIndex
) noexcept {
	VkImageBarrier().AddOwnershipBarrier(
		m_resource.GetResource(), VK_IMAGE_ASPECT_COLOR_BIT,
		oldOwnerQueueIndex, newOwnerQueueIndex,
		VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_NONE,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	).RecordBarriers(copyCmdBuffer);
}

void VkImageResourceView::AcquireOwnership(
	VkCommandBuffer cmdBuffer, std::uint32_t oldOwnerQueueIndex,
	std::uint32_t newOwnerQueueIndex, VkAccessFlagBits2 destinationAccess,
	VkPipelineStageFlagBits2 destinationStage
) noexcept {
	VkImageBarrier().AddOwnershipBarrier(
		m_resource.GetResource(), VK_IMAGE_ASPECT_COLOR_BIT,
		oldOwnerQueueIndex, newOwnerQueueIndex,
		VK_ACCESS_2_NONE, destinationAccess,
		VK_PIPELINE_STAGE_2_NONE, destinationStage,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	).RecordBarriers(cmdBuffer);
}

VkImageView VkImageResourceView::GetImageView() const noexcept {
	return m_imageView;
}

VkImage VkImageResourceView::GetResource() const noexcept {
	return m_resource.GetResource();
}

VkMemoryRequirements VkImageResourceView::GetMemoryRequirements(VkDevice device) const noexcept {
	return m_resource.GetMemoryRequirements(device);
}

VkDeviceSize VkImageResourceView::GetMemoryOffset() const noexcept {
	return m_memoryOffset;
}

// VK Uploadable Buffer ResourceView
VkUploadableBufferResourceView::VkUploadableBufferResourceView(VkDevice device) noexcept
	: VkUploadableResourceView<VkResourceView>{ device } {}

VkUploadableBufferResourceView::VkUploadableBufferResourceView(
	VkUploadableBufferResourceView&& resourceView
) noexcept : VkUploadableResourceView<VkResourceView>{ std::move(resourceView) } {}

VkUploadableBufferResourceView& VkUploadableBufferResourceView::operator=(
	VkUploadableBufferResourceView&& resourceView
	) noexcept {
	m_uploadResource = std::move(resourceView.m_uploadResource);
	m_gpuResource = std::move(resourceView.m_gpuResource);

	return *this;
}

void VkUploadableBufferResourceView::CreateResource(
	VkDevice device, VkDeviceSize bufferSize, std::uint32_t subAllocationCount,
	VkBufferUsageFlags gpuBufferType, std::vector<std::uint32_t> queueFamilyIndices
) {
	m_uploadResource.CreateResource(
		device, bufferSize, subAllocationCount, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
	);
	m_gpuResource.CreateResource(
		device, bufferSize, subAllocationCount,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | gpuBufferType, queueFamilyIndices
	);
}

VkDeviceSize VkUploadableBufferResourceView::GetSubAllocationSize() const noexcept {
	return m_gpuResource.GetSubAllocationSize();
}

VkDeviceSize VkUploadableBufferResourceView::GetSubAllocationOffset(
	VkDeviceSize index
) const noexcept {
	return m_gpuResource.GetSubAllocationOffset(index);
}

// VK Uploadable Image ResourceView
VkUploadableImageResourceView::VkUploadableImageResourceView(VkDevice device) noexcept
	: VkUploadableResourceView<VkImageResourceView>{ device } {}

VkUploadableImageResourceView::VkUploadableImageResourceView(
	VkUploadableImageResourceView&& resourceView
) noexcept : VkUploadableResourceView<VkImageResourceView>{ std::move(resourceView) } {}

VkUploadableImageResourceView& VkUploadableImageResourceView::operator=(
	VkUploadableImageResourceView&& resourceView
	) noexcept {
	m_uploadResource = std::move(resourceView.m_uploadResource);
	m_gpuResource = std::move(resourceView.m_gpuResource);

	return *this;
}

void VkUploadableImageResourceView::CreateResource(
	VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
	std::vector<std::uint32_t> queueFamilyIndices
) {
	// Upload Buffer requires the size of Data to be copied because it's going to memcpy
	// with that size
	m_uploadResource.CreateResource(
		device, static_cast<VkDeviceSize>(width * height * 4u), 1u,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT
	);
	m_gpuResource.CreateResource(
		device, width, height, imageFormat,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		queueFamilyIndices
	);
}

void VkUploadableImageResourceView::CreateImageView(
	VkDevice device, VkImageAspectFlagBits aspectBit
) {
	m_gpuResource.CreateImageView(device, aspectBit);
}

void VkUploadableImageResourceView::TransitionImageLayout(VkCommandBuffer cmdBuffer) noexcept {
	VkImageBarrier().AddLayoutBarrier(
		m_gpuResource.GetResource(), VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
		VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
	).RecordBarriers(cmdBuffer);
}

VkImageView VkUploadableImageResourceView::GetImageView() const noexcept {
	return m_gpuResource.GetImageView();
}
