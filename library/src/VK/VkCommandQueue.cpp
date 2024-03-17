#include <VkCommandQueue.hpp>
#include <VkResourceBarriers2.hpp>

// Command Buffer
VKCommandBuffer::VKCommandBuffer() : m_commandBuffer{VK_NULL_HANDLE} {}
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

VKCommandBuffer& VKCommandBuffer::Reset() noexcept
{
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

	return *this;
}

VKCommandBuffer& VKCommandBuffer::Close() noexcept
{
	vkEndCommandBuffer(m_commandBuffer);

	return *this;
}

VKCommandBuffer& VKCommandBuffer::Copy(
	const Buffer& src, const Buffer& dst, const BufferToBufferCopyBuilder& builder
) noexcept {
	vkCmdCopyBuffer(m_commandBuffer, src.Get(), dst.Get(), 1u, builder.GetPtr());

	return *this;
}

VKCommandBuffer& VKCommandBuffer::Copy(
	const Buffer& src, const Buffer& dst, BufferToBufferCopyBuilder&& builder
) noexcept {
	return Copy(src, dst, builder.Size(src.Size()));
}

VKCommandBuffer& VKCommandBuffer::Copy(
	const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder&& builder
) noexcept {
	return Copy(
		src, dst, builder.ImageExtent(dst.GetTexture().GetExtent()).ImageAspectFlags(dst.GetAspect())
	);
}

VKCommandBuffer& VKCommandBuffer::Copy(
	const Buffer& src, const VkTextureView& dst, const BufferToImageCopyBuilder& builder
) noexcept {
	VkImageBarrier2{}.AddMemoryBarrier(
		ImageBarrierBuilder{}
		.Image(dst)
		.Layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		.AccessMasks(VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT)
		.StageMasks(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT)
	).RecordBarriers(m_commandBuffer);

	vkCmdCopyBufferToImage(
		m_commandBuffer, src.Get(), dst.GetTexture().Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1u, builder.GetPtr()
	);

	return *this;
}

VKCommandBuffer& VKCommandBuffer::AcquireOwnership(
	const Buffer& buffer,
	const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
	VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage
) noexcept {
	VkBufferBarrier2{}.AddMemoryBarrier(
		BufferBarrierBuilder{}
		.Buffer(buffer)
		.QueueIndices(srcQueue.GetFamilyIndex(), dstQueue.GetFamilyIndex())
		.AccessMasks(VK_ACCESS_NONE, dstAccess)
		.StageMasks(VK_PIPELINE_STAGE_NONE, dstStage)
	).RecordBarriers(m_commandBuffer);

	return *this;
}

VKCommandBuffer& VKCommandBuffer::AcquireOwnership(
	const VkTextureView& textureView,
	const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
	VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage,
	VkImageLayout oldLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */,
	VkImageLayout newLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */
) noexcept {
	VkImageBarrier2().AddMemoryBarrier(
		ImageBarrierBuilder{}
		.Image(textureView)
		.QueueIndices(srcQueue.GetFamilyIndex(), dstQueue.GetFamilyIndex())
		.AccessMasks(VK_ACCESS_NONE, dstAccess)
		.Layouts(oldLayout, newLayout)
		.StageMasks(VK_PIPELINE_STAGE_NONE, dstStage)
	).RecordBarriers(m_commandBuffer);

	return *this;
}

VKCommandBuffer& VKCommandBuffer::ReleaseOwnership(
	const Buffer& buffer, const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue
) noexcept {
	VkBufferBarrier2{}.AddMemoryBarrier(
		BufferBarrierBuilder{}
		.Buffer(buffer)
		.QueueIndices(srcQueue.GetFamilyIndex(), dstQueue.GetFamilyIndex())
		.AccessMasks(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE)
		.StageMasks(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_NONE)
	).RecordBarriers(m_commandBuffer);

	return *this;
}

VKCommandBuffer& VKCommandBuffer::ReleaseOwnership(
	const VkTextureView& textureView, const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
	VkImageLayout oldLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */,
	VkImageLayout newLayout /* = VK_IMAGE_LAYOUT_UNDEFINED */
) noexcept {
	VkImageBarrier2{}.AddMemoryBarrier(
		ImageBarrierBuilder{}
		.Image(textureView)
		.QueueIndices(srcQueue.GetFamilyIndex(), dstQueue.GetFamilyIndex())
		.AccessMasks(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE)
		.Layouts(oldLayout, newLayout)
		.StageMasks(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_NONE)
	).RecordBarriers(m_commandBuffer);

	return *this;
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
		{
			VKFence fence{ m_device };
			// The first fence needs to be created signaled since in a Render Loop I will
			// check if a fence is signaled or not at the very beginning and it will stay
			// blocked there if the fence wasn't created signaled as no queue will be signalling
			// it.
			fence.Create(true);
			m_fences.emplace_back(std::move(fence));
		}

		for (std::uint32_t _ = 1u; _ < bufferCount; ++_)
		{
			VKFence fence{ m_device };
			fence.Create(false);
			m_fences.emplace_back(std::move(fence));
		}
	}
}

void VkGraphicsQueue::WaitForSubmission(size_t bufferIndex)
{
	m_fences.at(bufferIndex).Wait();

	QueueExecutionFinishedEvent finishEvent{ static_cast<std::uint8_t>(bufferIndex) };
	std::future<void> waitObj = m_eventDispatcher->DispatchFeedback(finishEvent);
	waitObj.wait();
}
