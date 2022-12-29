#include <VkResourceViews.hpp>
#include <VkResourceBarriers.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

// _vkResourceView
VkDeviceSize _vkResourceView::s_uniformBufferAlignment = 0u;
VkDeviceSize _vkResourceView::s_storageBufferAlignment = 0u;

_vkResourceView::_vkResourceView(VkDevice device) noexcept
	: m_resource{ device }, m_memoryOffsetStart{ 0u }, m_bufferSize{ 0u },
	m_resourceType{ MemoryType::none } {}

_vkResourceView::_vkResourceView(_vkResourceView&& resourceView) noexcept
	: m_resource{ std::move(resourceView.m_resource) },
	m_memoryOffsetStart{ resourceView.m_memoryOffsetStart },
	m_bufferSize{ resourceView.m_bufferSize }, m_resourceType{ resourceView.m_resourceType } {}

_vkResourceView& _vkResourceView::operator=(_vkResourceView&& resourceView) noexcept {
	m_resource = std::move(resourceView.m_resource);
	m_memoryOffsetStart = resourceView.m_memoryOffsetStart;
	m_bufferSize = resourceView.m_bufferSize;
	m_resourceType = resourceView.m_resourceType;

	return *this;
}

void _vkResourceView::BindResourceToMemory(VkDevice device) const noexcept {
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

void _vkResourceView::SetMemoryOffsetAndType(VkDevice device, MemoryType type) noexcept {
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

void _vkResourceView::SetMemoryOffsetAndType(VkDeviceSize offset, MemoryType type) noexcept {
	m_memoryOffsetStart = offset;
	m_resourceType = type;
}

void _vkResourceView::CleanUpResource() noexcept {
	m_resource.CleanUpResource();
}

void _vkResourceView::RecordCopy(
	VkCommandBuffer transferCmdBuffer, const _vkResourceView& uploadBuffer
) noexcept {
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0u;
	copyRegion.dstOffset = 0u;
	copyRegion.size = uploadBuffer.GetBufferSize();

	vkCmdCopyBuffer(
		transferCmdBuffer, uploadBuffer.GetResource(), m_resource.GetResource(), 1u, &copyRegion
	);
}

void _vkResourceView::ReleaseOwnerShip(
	VkCommandBuffer transferCmdBuffer, std::uint32_t oldOwnerQueueIndex,
	std::uint32_t newOwnerQueueIndex
) noexcept {
	// Destination doesn't matter in release but needs to be there because of limitation
	VkBufferBarrier().AddMemoryBarrier(
		m_resource.GetResource(), m_bufferSize, 0u,
		oldOwnerQueueIndex, newOwnerQueueIndex,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
	).RecordBarriers(
		transferCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
	);
}

void _vkResourceView::AcquireOwnership(
	VkCommandBuffer cmdBuffer, std::uint32_t oldOwnerQueueIndex,
	std::uint32_t newOwnerQueueIndex, VkAccessFlagBits destinationAccess,
	VkPipelineStageFlagBits destinationStage
) noexcept {
	// Source doesn't matter in acquire but needs to be there because of limitation
	VkBufferBarrier().AddMemoryBarrier(
		m_resource.GetResource(), m_bufferSize, 0u,
		oldOwnerQueueIndex, newOwnerQueueIndex,
		VK_ACCESS_TRANSFER_WRITE_BIT, destinationAccess
	).RecordBarriers(
		cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationStage
	);
}

void _vkResourceView::SetBufferAlignments(VkPhysicalDevice device) noexcept {
	VkPhysicalDeviceProperties deviceProperty{};
	vkGetPhysicalDeviceProperties(device, &deviceProperty);

	s_uniformBufferAlignment = deviceProperty.limits.minUniformBufferOffsetAlignment;
	s_storageBufferAlignment = deviceProperty.limits.minStorageBufferOffsetAlignment;
}

VkBuffer _vkResourceView::GetResource() const noexcept {
	return m_resource.GetResource();
}

VkDeviceSize _vkResourceView::GetBufferSize() const noexcept {
	return m_bufferSize;
}

VkMemoryRequirements _vkResourceView::GetMemoryRequirements(VkDevice device) const noexcept {
	return m_resource.GetMemoryRequirements(device);
}

// Vk Resource view
VkResourceView::VkResourceView(VkDevice device) noexcept
	: _vkResourceView{ device }, m_subAllocationSize{ 0u }, m_subBufferSize{ 0u } {}

VkResourceView::VkResourceView(VkResourceView&& resourceView) noexcept
	: _vkResourceView{ std::move(resourceView) },
	m_subAllocationSize{ resourceView.m_subAllocationSize },
	m_subBufferSize{ resourceView.m_subBufferSize } {}

VkResourceView& VkResourceView::operator=(VkResourceView&& resourceView) noexcept {
	m_resource = std::move(resourceView.m_resource);
	m_memoryOffsetStart = resourceView.m_memoryOffsetStart;
	m_bufferSize = resourceView.m_bufferSize;
	m_resourceType = resourceView.m_resourceType;
	m_subAllocationSize = resourceView.m_subAllocationSize;
	m_subBufferSize = resourceView.m_subBufferSize;

	return *this;
}

void VkResourceView::CreateResource(
	VkDevice device, VkDeviceSize bufferSize, std::uint32_t subAllocationCount,
	VkBufferUsageFlags usageFlags, std::vector<std::uint32_t> queueFamilyIndices
) {
	if(usageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
		m_subAllocationSize = Align(bufferSize, s_uniformBufferAlignment);
	else if (usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
		m_subAllocationSize = Align(bufferSize, s_storageBufferAlignment);
	else
		m_subAllocationSize = bufferSize;

	m_subBufferSize = bufferSize;

	m_bufferSize = m_subAllocationSize * static_cast<VkDeviceSize>(subAllocationCount - 1u)
		+ m_subBufferSize;
	m_resource.CreateResource(device, m_bufferSize, usageFlags, queueFamilyIndices);
}

std::vector<VkDescriptorBufferInfo> VkResourceView::GetDescBufferInfoSpread(
	size_t bufferCount
) const noexcept {
	std::vector<VkDescriptorBufferInfo> bufferInfos;

	for (size_t _ = 0u; _ < bufferCount; ++_) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = GetResource();
		bufferInfo.offset = GetFirstSubAllocationOffset();
		bufferInfo.range = GetSubBufferSize();

		bufferInfos.emplace_back(bufferInfo);
	}

	return bufferInfos;
}

std::vector<VkDescriptorBufferInfo> VkResourceView::GetDescBufferInfoSplit(
	size_t bufferCount
) const noexcept {
	std::vector<VkDescriptorBufferInfo> bufferInfos;

	for (VkDeviceSize index = 0u; index < bufferCount; ++index) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = GetResource();
		bufferInfo.offset = GetSubAllocationOffset(index);
		bufferInfo.range = GetSubBufferSize();

		bufferInfos.emplace_back(bufferInfo);
	}

	return bufferInfos;
}

VkDeviceSize VkResourceView::GetSubAllocationOffset(VkDeviceSize index) const noexcept {
	return m_subAllocationSize * index;
}

VkDeviceSize VkResourceView::GetMemoryOffset(VkDeviceSize index) const noexcept {
	return m_memoryOffsetStart + GetSubAllocationOffset(index);
}

VkDeviceSize VkResourceView::GetFirstMemoryOffset() const noexcept {
	return GetMemoryOffset(0u);
}

VkDeviceSize VkResourceView::GetFirstSubAllocationOffset() const noexcept {
	return GetSubAllocationOffset(0u);
}

VkDeviceSize VkResourceView::GetSubBufferSize() const noexcept {
	return m_subBufferSize;
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

void VkImageResourceView::BindResourceToMemory(VkDevice device) const noexcept {
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
	VkCommandBuffer transferCmdBuffer, const _vkResourceView& uploadBuffer
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

	VkImageBarrier().AddExecutionBarrier(
		m_resource.GetResource(), VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT
	).RecordBarriers(
		transferCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
	);

	vkCmdCopyBufferToImage(
		transferCmdBuffer, uploadBuffer.GetResource(),
		m_resource.GetResource(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1u, &copyRegion
	);
}

void VkImageResourceView::ReleaseOwnerShip(
	VkCommandBuffer transferCmdBuffer, std::uint32_t oldOwnerQueueIndex,
	std::uint32_t newOwnerQueueIndex
) noexcept {
	// Destination doesn't matter in release but needs to be there because of limitation
	VkImageBarrier().AddMemoryBarrier(
		m_resource.GetResource(), VK_IMAGE_ASPECT_COLOR_BIT,
		oldOwnerQueueIndex, newOwnerQueueIndex,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	).RecordBarriers(
		transferCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
	);
}

void VkImageResourceView::AcquireOwnership(
	VkCommandBuffer cmdBuffer, std::uint32_t oldOwnerQueueIndex,
	std::uint32_t newOwnerQueueIndex, VkAccessFlagBits destinationAccess,
	VkPipelineStageFlagBits destinationStage
) noexcept {
	// Source doesn't matter in acquire but needs to be there because of limitation
	VkImageBarrier().AddMemoryBarrier(
		m_resource.GetResource(), VK_IMAGE_ASPECT_COLOR_BIT,
		oldOwnerQueueIndex, newOwnerQueueIndex,
		VK_ACCESS_TRANSFER_WRITE_BIT, destinationAccess,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	).RecordBarriers(
		cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationStage
	);
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

std::vector<VkDescriptorBufferInfo> VkUploadableBufferResourceView::GetDescBufferInfoSpread(
	size_t bufferCount
) const noexcept {
	return m_gpuResource.GetDescBufferInfoSpread(bufferCount);
}

std::vector<VkDescriptorBufferInfo> VkUploadableBufferResourceView::GetDescBufferInfoSplit(
	size_t bufferCount
) const noexcept {
	return m_gpuResource.GetDescBufferInfoSplit(bufferCount);
}

VkDeviceSize VkUploadableBufferResourceView::GetSubAllocationOffset(
	VkDeviceSize index
) const noexcept {
	return m_gpuResource.GetSubAllocationOffset(index);
}

VkDeviceSize VkUploadableBufferResourceView::GetFirstSubAllocationOffset() const noexcept {
	return GetSubAllocationOffset(0u);
}

VkDeviceSize VkUploadableBufferResourceView::GetSubBufferSize() const noexcept {
	return m_gpuResource.GetSubBufferSize();
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
	VkImageBarrier().AddExecutionBarrier(
		m_gpuResource.GetResource(), VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
	).RecordBarriers(
		cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	);
}

VkImageView VkUploadableImageResourceView::GetImageView() const noexcept {
	return m_gpuResource.GetImageView();
}

// Vk Argument ResourceView
VkArgumentResourceView::VkArgumentResourceView(VkDevice device) noexcept
	: _vkResourceView{ device } {}

VkArgumentResourceView::VkArgumentResourceView(VkArgumentResourceView&& resourceView) noexcept
	: _vkResourceView{ std::move(resourceView) } {}

VkArgumentResourceView& VkArgumentResourceView::operator=(
	VkArgumentResourceView&& resourceView
) noexcept {
	m_resource = std::move(resourceView.m_resource);
	m_memoryOffsetStart = resourceView.m_memoryOffsetStart;
	m_bufferSize = resourceView.m_bufferSize;
	m_resourceType = resourceView.m_resourceType;

	return *this;
}

void VkArgumentResourceView::CreateResource(
	VkDevice device, VkDeviceSize bufferSize, std::vector<std::uint32_t> queueFamilyIndices
) {

	m_bufferSize = s_storageBufferAlignment + bufferSize;
	// 4bytes for counter buffer which will be created at offset 0 then allocate the
	// actual buffer at the alignment

	m_resource.CreateResource(
		device, m_bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		| VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		queueFamilyIndices
	);
}

VkArgumentResourceView::BufferInfoType VkArgumentResourceView::GetDescBufferInfo(
	size_t bufferCount, const std::vector<VkArgumentResourceView>& buffers
) noexcept {
	std::vector<VkDescriptorBufferInfo> resourceBufferInfos;
	std::vector<VkDescriptorBufferInfo> counterBufferInfos;

	for (size_t index = 0u; index < bufferCount; ++index) {
		auto& argumentBuffer = buffers[index];

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = argumentBuffer.GetResource();
		bufferInfo.offset = argumentBuffer.GetBufferOffset();
		bufferInfo.range = argumentBuffer.GetResourceBufferSize();

		resourceBufferInfos.emplace_back(bufferInfo);

		bufferInfo.offset = argumentBuffer.GetCounterOffset();
		bufferInfo.range = argumentBuffer.GetCounterBufferSize();

		counterBufferInfos.emplace_back(bufferInfo);
	}

	return { resourceBufferInfos, counterBufferInfos };
}

VkDeviceSize VkArgumentResourceView::GetCounterOffset() const noexcept {
	return 0u;
}

VkDeviceSize VkArgumentResourceView::GetBufferOffset() const noexcept {
	return s_storageBufferAlignment;
}

VkDeviceSize VkArgumentResourceView::GetBufferMemoryOffset() const noexcept {
	return m_memoryOffsetStart + GetBufferOffset();
}

VkDeviceSize VkArgumentResourceView::GetCounterMemoryOffset() const noexcept {
	return m_memoryOffsetStart + GetCounterOffset();
}

VkDeviceSize VkArgumentResourceView::GetCounterBufferSize() const noexcept {
	return 4u;
}

VkDeviceSize VkArgumentResourceView::GetResourceBufferSize() const noexcept {
	return m_bufferSize - s_storageBufferAlignment;
}
