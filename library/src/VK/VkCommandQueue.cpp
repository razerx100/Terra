#include <VkCommandQueue.hpp>
#include <VkResourceBarriers2.hpp>

namespace Terra
{
// Command Buffer
VKCommandBuffer::VKCommandBuffer() : m_commandBuffer{ VK_NULL_HANDLE } {}
VKCommandBuffer::VKCommandBuffer(VkDevice device, VkCommandPool commandPool) : VKCommandBuffer{}
{
	Create(device, commandPool);
}

void VKCommandBuffer::Create(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocateInfo{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = commandPool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1u
	};

	vkAllocateCommandBuffers(device, &allocateInfo, &m_commandBuffer);
}


void VKCommandBuffer::Begin() const noexcept
{
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
}

void VKCommandBuffer::Reset() const noexcept
{
	vkResetCommandBuffer(m_commandBuffer, 0u);
}

void VKCommandBuffer::Close() const noexcept
{
	vkEndCommandBuffer(m_commandBuffer);
}

void VKCommandBuffer::Copy(
	const Buffer& src, const Buffer& dst, const BufferToBufferCopyBuilder& builder
) const noexcept {
	vkCmdCopyBuffer(m_commandBuffer, src.Get(), dst.Get(), 1u, builder.GetPtr());
}

void VKCommandBuffer::CopyWhole(
	const Buffer& src, const Buffer& dst, BufferToBufferCopyBuilder& builder
) const noexcept {
	Copy(src, dst, builder.Size(src.BufferSize()));
}

void VKCommandBuffer::CopyWithoutBarrier(
	const Buffer& src, const VkTextureView& dst, const BufferToImageCopyBuilder& builder
) const noexcept {
	vkCmdCopyBufferToImage(
		m_commandBuffer, src.Get(), dst.GetTexture().Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1u, builder.GetPtr()
	);
}

void VKCommandBuffer::CopyWholeWithoutBarrier(
	const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder& builder
) const noexcept {
	CopyWithoutBarrier(
		src, dst, builder.ImageExtent(dst.GetTexture().GetExtent()).ImageAspectFlags(dst.GetAspect())
	);
}

void VKCommandBuffer::CopyWhole(
	const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder& builder
) const noexcept {
	Copy(
		src, dst, builder.ImageExtent(dst.GetTexture().GetExtent()).ImageAspectFlags(dst.GetAspect())
	);
}

void VKCommandBuffer::Copy(
	const Buffer& src, const VkTextureView& dst, const BufferToImageCopyBuilder& builder
) const noexcept {
	VkImageBarrier2{}.AddMemoryBarrier(
		ImageBarrierBuilder{}
		.Image(dst)
		.Layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		.AccessMasks(VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT)
		.StageMasks(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT)
	).RecordBarriers(m_commandBuffer);

	CopyWithoutBarrier(src, dst, builder);
}

void VKCommandBuffer::Copy(
	const VKImageView& src, const VKImageView& dst, const ImageCopyBuilder& builder
) const noexcept {
	vkCmdCopyImage(
		m_commandBuffer,
		src.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1u, builder.GetPtr()
	);
}

void VKCommandBuffer::AcquireOwnership(
	const Buffer& buffer,
	std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
	VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage
) const noexcept {
	VkBufferBarrier2{}.AddMemoryBarrier(
		BufferBarrierBuilder{}
		.Buffer(buffer)
		.QueueIndices(srcQueueFamilyIndex, dstQueueFamilyIndex)
		.AccessMasks(VK_ACCESS_NONE, dstAccess)
		.StageMasks(VK_PIPELINE_STAGE_NONE, dstStage)
	).RecordBarriers(m_commandBuffer);
}

void VKCommandBuffer::AcquireOwnership(
	const VkTextureView& textureView,
	std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
	VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage,
	VkImageLayout oldLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */,
	VkImageLayout newLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */
) const noexcept {
	VkImageBarrier2().AddMemoryBarrier(
		ImageBarrierBuilder{}
		.Image(textureView)
		.QueueIndices(srcQueueFamilyIndex, dstQueueFamilyIndex)
		.AccessMasks(VK_ACCESS_NONE, dstAccess)
		.Layouts(oldLayout, newLayout)
		.StageMasks(VK_PIPELINE_STAGE_NONE, dstStage)
	).RecordBarriers(m_commandBuffer);
}

void VKCommandBuffer::ReleaseOwnership(
	const Buffer& buffer,
	std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex
) const noexcept {
	VkBufferBarrier2{}.AddMemoryBarrier(
		BufferBarrierBuilder{}
		.Buffer(buffer)
		.QueueIndices(srcQueueFamilyIndex, dstQueueFamilyIndex)
		.AccessMasks(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE)
		.StageMasks(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_NONE)
	).RecordBarriers(m_commandBuffer);
}

void VKCommandBuffer::ReleaseOwnership(
	const VkTextureView& textureView,
	std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
	VkImageLayout oldLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */,
	VkImageLayout newLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */
) const noexcept {
	VkImageBarrier2{}.AddMemoryBarrier(
		ImageBarrierBuilder{}
		.Image(textureView)
		.QueueIndices(srcQueueFamilyIndex, dstQueueFamilyIndex)
		.AccessMasks(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE)
		.Layouts(oldLayout, newLayout)
		.StageMasks(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_NONE)
	).RecordBarriers(m_commandBuffer);
}

void VKCommandBuffer::AcquireOwnership(
	const Buffer& buffer,
	const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
	VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage
) const noexcept {
	AcquireOwnership(buffer, srcQueue.GetFamilyIndex(), dstQueue.GetFamilyIndex(), dstAccess, dstStage);
}

void VKCommandBuffer::AcquireOwnership(
	const VkTextureView& textureView,
	const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
	VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage,
	VkImageLayout oldLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */ ,
	VkImageLayout newLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */
) const noexcept {
	AcquireOwnership(
		textureView, srcQueue.GetFamilyIndex(), dstQueue.GetFamilyIndex(), dstAccess, dstStage,
		oldLayout, newLayout
	);
}

void VKCommandBuffer::ReleaseOwnership(
	const Buffer& buffer,
	const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue
) const noexcept {
	ReleaseOwnership(buffer, srcQueue.GetFamilyIndex(), dstQueue.GetFamilyIndex());
}

void VKCommandBuffer::ReleaseOwnership(
	const VkTextureView& textureView,
	const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
	VkImageLayout oldLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */,
	VkImageLayout newLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */
) const noexcept {
	ReleaseOwnership(
		textureView, srcQueue.GetFamilyIndex(), dstQueue.GetFamilyIndex(), oldLayout, newLayout
	);
}

// Command Queue
VkCommandQueue::~VkCommandQueue() noexcept
{
	SelfDestruct();
}

void VkCommandQueue::SelfDestruct() noexcept
{
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

void VkCommandQueue::CreateCommandBuffers(std::uint32_t bufferCount)
{
	VkCommandPoolCreateInfo poolInfo{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_queueIndex
	};

	vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

	for (std::uint32_t _ = 0u; _ < bufferCount; ++_)
		m_commandBuffers.emplace_back(m_device, m_commandPool);
}

// Graphics Queue
void VkGraphicsQueue::CreateCommandBuffers(std::uint32_t bufferCount)
{
	VkCommandQueue::CreateCommandBuffers(bufferCount);

	if (bufferCount)
	{
		// The fences need to be created signaled since in a Render Loop I will
		// check if a fence is signaled or not at the very beginning and it will stay
		// blocked there if the fence wasn't created signaled as no queue will be signalling
		// it.

		for (std::uint32_t _ = 0u; _ < bufferCount; ++_)
		{
			VKFence fence{ m_device };
			fence.Create(true);
			m_fences.emplace_back(std::move(fence));
		}
	}
}

void VkGraphicsQueue::WaitForSubmission(size_t bufferIndex)
{
	m_fences[bufferIndex].Wait();
}

void VkGraphicsQueue::WaitForQueueToFinish()
{
	for (auto& fence : m_fences)
		fence.Wait();
}
}
