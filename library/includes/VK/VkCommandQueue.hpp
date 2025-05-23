#ifndef VK_COMMAND_QUEUE_HPP_
#define VK_COMMAND_QUEUE_HPP_
#include <vulkan/vulkan.hpp>
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkResourceBarriers2.hpp>
#include <VkSyncObjects.hpp>
#include <array>
#include <utility>
#include <cassert>

namespace Terra
{
class BufferToBufferCopyBuilder
{
public:
	BufferToBufferCopyBuilder()
		: m_bufferCopy{
			.srcOffset = 0u,
			.dstOffset = 0u,
			.size      = 0u
		}
	{}

	BufferToBufferCopyBuilder& Size(VkDeviceSize size) noexcept
	{
		m_bufferCopy.size = size;

		return *this;
	}

	BufferToBufferCopyBuilder& SrcOffset(VkDeviceSize offset) noexcept
	{
		m_bufferCopy.srcOffset = offset;

		return *this;
	}

	BufferToBufferCopyBuilder& DstOffset(VkDeviceSize offset) noexcept
	{
		m_bufferCopy.dstOffset = offset;

		return *this;
	}

	[[nodiscard]]
	const VkBufferCopy* GetPtr() const noexcept { return &m_bufferCopy; }
	[[nodiscard]]
	VkBufferCopy Get() const noexcept { return m_bufferCopy; }

private:
	VkBufferCopy m_bufferCopy;
};

class BufferToImageCopyBuilder
{
public:
	BufferToImageCopyBuilder()
		: m_imageCopy{
			.bufferOffset      = 0u,
			.bufferRowLength   = 0u,
			.bufferImageHeight = 0u,
			.imageSubresource  = VkImageSubresourceLayers{ 0u, 0u, 0u, 1u },
			.imageOffset       = VkOffset3D{ .x = 0u, .y = 0u, .z = 0u },
			.imageExtent       = VkExtent3D{ .width = 0u, .height = 0u, .depth = 0u }
		}
	{}

	BufferToImageCopyBuilder& ImageOffset(
		std::uint32_t x, std::uint32_t y, std::uint32_t z = 0u
	) noexcept {
		m_imageCopy.imageOffset.x = x;
		m_imageCopy.imageOffset.y = y;
		m_imageCopy.imageOffset.z = z;

		return *this;
	}

	BufferToImageCopyBuilder& ImageOffset(const VkOffset3D& offset) noexcept
	{
		m_imageCopy.imageOffset = offset;

		return *this;
	}

	BufferToImageCopyBuilder& ImageExtent(
		std::uint32_t width, std::uint32_t height, std::uint32_t depth = 0u
	) noexcept {
		m_imageCopy.imageExtent.width  = width;
		m_imageCopy.imageExtent.height = height;
		m_imageCopy.imageExtent.depth  = depth;

		return *this;
	}

	BufferToImageCopyBuilder& ImageExtent(const VkExtent3D& extent) noexcept
	{
		m_imageCopy.imageExtent = extent;

		return *this;
	}

	BufferToImageCopyBuilder& ImageAspectFlags(VkImageAspectFlags aspectFlags) noexcept
	{
		m_imageCopy.imageSubresource.aspectMask = aspectFlags;

		return *this;
	}

	// The MipLevel to copy.
	BufferToImageCopyBuilder& ImageMipLevel(std::uint32_t mipLevel) noexcept
	{
		m_imageCopy.imageSubresource.mipLevel = mipLevel;

		return *this;
	}

	BufferToImageCopyBuilder& BufferOffet(VkDeviceSize offset) noexcept
	{
		m_imageCopy.bufferOffset = offset;

		return *this;
	}

	BufferToImageCopyBuilder& BufferRowAndHeight(std::uint32_t row, std::uint32_t height) noexcept
	{
		m_imageCopy.bufferRowLength   = row;
		m_imageCopy.bufferImageHeight = height;

		return *this;
	}

	[[nodiscard]]
	const VkBufferImageCopy* GetPtr() const noexcept { return &m_imageCopy; }
	[[nodiscard]]
	VkBufferImageCopy Get() const noexcept { return m_imageCopy; }

private:
	VkBufferImageCopy m_imageCopy;
};

class ImageCopyBuilder
{
public:
	ImageCopyBuilder()
		: m_imageCopy
		{
			.srcSubresource = VkImageSubresourceLayers{ 0u, 0u, 0u, 1u },
			.srcOffset      = VkOffset3D{ .x = 0u, .y = 0u, .z = 0u },
			.dstSubresource = VkImageSubresourceLayers{ 0u, 0u, 0u, 1u },
			.dstOffset      = VkOffset3D{ .x = 0u, .y = 0u, .z = 0u },
			.extent         = VkExtent3D{ .width = 0u, .height = 0u, .depth = 0u }
		}
	{}

	ImageCopyBuilder& SrcImageOffset(
		std::uint32_t x, std::uint32_t y, std::uint32_t z = 0u
	) noexcept {
		m_imageCopy.srcOffset.x = x;
		m_imageCopy.srcOffset.y = y;
		m_imageCopy.srcOffset.z = z;

		return *this;
	}

	ImageCopyBuilder& SrcImageOffset(const VkOffset3D& offset) noexcept
	{
		m_imageCopy.srcOffset = offset;

		return *this;
	}

	ImageCopyBuilder& DstImageOffset(
		std::uint32_t x, std::uint32_t y, std::uint32_t z = 0u
	) noexcept {
		m_imageCopy.dstOffset.x = x;
		m_imageCopy.dstOffset.y = y;
		m_imageCopy.dstOffset.z = z;

		return *this;
	}

	ImageCopyBuilder& DstImageOffset(const VkOffset3D& offset) noexcept
	{
		m_imageCopy.dstOffset = offset;

		return *this;
	}

	ImageCopyBuilder& Extent(
		std::uint32_t width, std::uint32_t height, std::uint32_t depth = 0u
	) noexcept {
		m_imageCopy.extent.width  = width;
		m_imageCopy.extent.height = height;
		m_imageCopy.extent.depth  = depth;

		return *this;
	}

	ImageCopyBuilder& Extent(const VkExtent3D& extent) noexcept
	{
		m_imageCopy.extent = extent;

		return *this;
	}

	ImageCopyBuilder& SrcImageAspectFlags(VkImageAspectFlags aspectFlags) noexcept
	{
		m_imageCopy.srcSubresource.aspectMask = aspectFlags;

		return *this;
	}

	ImageCopyBuilder& DstImageAspectFlags(VkImageAspectFlags aspectFlags) noexcept
	{
		m_imageCopy.dstSubresource.aspectMask = aspectFlags;

		return *this;
	}

	[[nodiscard]]
	const VkImageCopy* GetPtr() const noexcept { return &m_imageCopy; }
	[[nodiscard]]
	VkImageCopy Get() const noexcept { return m_imageCopy; }

private:
	VkImageCopy m_imageCopy;
};

class VkCommandQueue;

class VKCommandBuffer
{
public:
	VKCommandBuffer();
	VKCommandBuffer(VkDevice device, VkCommandPool commandPool);

	void Create(VkDevice device, VkCommandPool commandPool);

	void Reset() const noexcept;
	void Begin() const noexcept;
	void Close() const noexcept;

	void Copy(
		const Buffer& src, const Buffer& dst, const BufferToBufferCopyBuilder& builder
	) const noexcept;
	void CopyWhole(
		const Buffer& src, const Buffer& dst, BufferToBufferCopyBuilder& builder
	) const noexcept;
	// Puts a barrier assuming that the texture is in undefined state.
	void Copy(
		const Buffer& src, const VkTextureView& dst, const BufferToImageCopyBuilder& builder
	) const noexcept;
	// Puts a barrier assuming that the texture is in undefined state.
	void CopyWhole(
		const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder& builder
	) const noexcept;

	void CopyWithoutBarrier(
		const Buffer& src, const VkTextureView& dst, const BufferToImageCopyBuilder& builder
	) const noexcept;
	void CopyWholeWithoutBarrier(
		const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder& builder
	) const noexcept;

	// The barriers should be handled before calling this.
	void Copy(
		const VKImageView& src, const VKImageView& dst, const ImageCopyBuilder& builder
	) const noexcept;
	void Copy(
		const VkTextureView& src, const VkTextureView& dst, const ImageCopyBuilder& builder
	) const noexcept {
		Copy(src.GetView(), dst.GetView(), builder);
	}

	void Copy(
		const Buffer& src, const Buffer& dst, BufferToBufferCopyBuilder&& builder
	) const noexcept {
		Copy(src, dst, builder);
	}
	void CopyWhole(
		const Buffer& src, const Buffer& dst, BufferToBufferCopyBuilder&& builder = {}
	) const noexcept
	{
		CopyWhole(src, dst, builder);
	}
	void Copy(
		const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder&& builder
	) const noexcept
	{
		Copy(src, dst, builder);
	}
	void CopyWhole(
		const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder&& builder = {}
	) const noexcept
	{
		CopyWhole(src, dst, builder);
	}

	void AcquireOwnership(
		const Buffer& buffer,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage
	) const noexcept;
	void AcquireOwnership(
		const VkTextureView& textureView,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED
	) const noexcept;
	void ReleaseOwnership(
		const Buffer& buffer, std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex
	) const noexcept;
	void ReleaseOwnership(
		const VkTextureView& textureView,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED
	) const noexcept;
	void AcquireOwnership(
		const Buffer& buffer,
		const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
		VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage
	) const noexcept;
	void AcquireOwnership(
		const VkTextureView& textureView,
		const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
		VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED
	) const noexcept;
	void ReleaseOwnership(
		const Buffer& buffer, const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue
	) const noexcept;
	void ReleaseOwnership(
		const VkTextureView& textureView,
		const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED
	) const noexcept;

	template<std::uint32_t BarrierCount = 1u>
	const VKCommandBuffer& AddBarrier(const VkBufferBarrier2<BarrierCount>& barrier) const noexcept
	{
		barrier.RecordBarriers(m_commandBuffer);

		return *this;
	}
	template<std::uint32_t BarrierCount = 1u>
	const VKCommandBuffer& AddBarrier(const VkImageBarrier2<BarrierCount>& barrier) const noexcept
	{
		barrier.RecordBarriers(m_commandBuffer);

		return *this;
	}

	[[nodiscard]]
	VkCommandBuffer Get() const noexcept { return m_commandBuffer; }

private:
	VkCommandBuffer m_commandBuffer;

public:
	VKCommandBuffer(const VKCommandBuffer&) = delete;
	VKCommandBuffer& operator=(const VKCommandBuffer&) = delete;

	VKCommandBuffer(VKCommandBuffer&& other) noexcept
		: m_commandBuffer{ std::exchange(other.m_commandBuffer, VK_NULL_HANDLE) }
	{}
	VKCommandBuffer& operator=(VKCommandBuffer&& other) noexcept
	{
		m_commandBuffer = std::exchange(other.m_commandBuffer, VK_NULL_HANDLE);

		return *this;
	}
};

// The command buffer will be reset at creation of an object and closed at destruction.
struct CommandBufferScope
{
	CommandBufferScope(const VKCommandBuffer& commandBuffer) : m_commandBuffer{ commandBuffer }
	{
		m_commandBuffer.Reset();
		m_commandBuffer.Begin();
	}

	~CommandBufferScope() noexcept { m_commandBuffer.Close(); }

	const VKCommandBuffer& m_commandBuffer;

	operator const VKCommandBuffer& () const noexcept { return m_commandBuffer; }
};

template<std::uint32_t WaitCount = 0u, std::uint32_t SignalCount = 0u, std::uint32_t CommandBufferCount = 1u>
class QueueSubmitBuilder
{
public:
	QueueSubmitBuilder()
		: m_waitSemaphores{}, m_waitStages{}, m_waitValues{},
		m_signalSemaphores{}, m_signalValues{}, m_commandBuffers{},
		m_timelineInfo{
			.sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			.waitSemaphoreValueCount   = WaitCount,
			.pWaitSemaphoreValues      = std::data(m_waitValues),
			.signalSemaphoreValueCount = SignalCount,
			.pSignalSemaphoreValues    = std::data(m_signalValues)
		},
		m_submitInfo{
			.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext                = &m_timelineInfo,
			.waitSemaphoreCount   = WaitCount,
			.pWaitSemaphores      = std::data(m_waitSemaphores),
			.pWaitDstStageMask    = std::data(m_waitStages),
			.commandBufferCount   = CommandBufferCount,
			.pCommandBuffers      = std::data(m_commandBuffers),
			.signalSemaphoreCount = SignalCount,
			.pSignalSemaphores    = std::data(m_signalSemaphores)
		},
		m_currentWaitIndex{ 0u }, m_currentSignalIndex{ 0u }, m_currentCmdBufferIndex{ 0u }
	{
		m_waitSemaphores.fill(VK_NULL_HANDLE);
		m_waitStages.fill(VK_PIPELINE_STAGE_NONE);
		m_waitValues.fill(0u);
		m_signalSemaphores.fill(VK_NULL_HANDLE);
		m_signalValues.fill(0u);
		m_commandBuffers.fill(VK_NULL_HANDLE);
	}

	QueueSubmitBuilder& WaitSemaphore(
		VkSemaphore semaphore, VkPipelineStageFlagBits pipelineStage,
		std::uint64_t waitValue = 1u
	) {
		assert(m_currentWaitIndex < WaitCount && "More Wait semaphores than the allowed amount.");

		m_waitSemaphores[m_currentWaitIndex] = semaphore;
		m_waitStages[m_currentWaitIndex]     = pipelineStage;
		m_waitValues[m_currentWaitIndex]     = waitValue;

		++m_currentWaitIndex;

		return *this;
	}
	QueueSubmitBuilder& SignalSemaphore(VkSemaphore semaphore, std::uint64_t signalValue = 1u)
	{
		assert(m_currentSignalIndex < SignalCount && "More Signal semaphores than the allowed amount.");

		m_signalSemaphores[m_currentSignalIndex] = semaphore;
		m_signalValues[m_currentSignalIndex]     = signalValue;

		++m_currentSignalIndex;

		return *this;
	}
	QueueSubmitBuilder& WaitSemaphore(
		const VKSemaphore& semaphore, VkPipelineStageFlagBits pipelineStage,
		std::uint64_t waitValue = 1u
	) {
		return WaitSemaphore(semaphore.Get(), pipelineStage, waitValue);
	}
	QueueSubmitBuilder& SignalSemaphore(const VKSemaphore& semaphore, std::uint64_t signalValue = 1u)
	{
		return SignalSemaphore(semaphore.Get(), signalValue);
	}
	QueueSubmitBuilder& CommandBuffer(const VKCommandBuffer& commandBuffer)
	{
		assert(
			m_currentCmdBufferIndex < CommandBufferCount
			&& "More CommandBuffers than the allowed amount."
		);

		m_commandBuffers[m_currentCmdBufferIndex] = commandBuffer.Get();

		++m_currentCmdBufferIndex;

		return *this;
	}

	[[nodiscard]]
	const VkSubmitInfo* GetPtr() const noexcept { return &m_submitInfo; }
	[[nodiscard]]
	VkSubmitInfo Get() const noexcept { return m_submitInfo; }

private:
	void UpdatePointers() noexcept
	{
		m_timelineInfo.pWaitSemaphoreValues   = std::data(m_waitValues);
		m_timelineInfo.pSignalSemaphoreValues = std::data(m_signalValues);

		m_submitInfo.pNext             = &m_timelineInfo;
		m_submitInfo.pWaitSemaphores   = std::data(m_waitSemaphores);
		m_submitInfo.pWaitDstStageMask = std::data(m_waitStages);
		m_submitInfo.pCommandBuffers   = std::data(m_commandBuffers);
		m_submitInfo.pSignalSemaphores = std::data(m_signalSemaphores);
	}

private:
	std::array<VkSemaphore, WaitCount>              m_waitSemaphores;
	std::array<VkPipelineStageFlags, WaitCount>     m_waitStages;
	std::array<std::uint64_t, WaitCount>            m_waitValues;
	std::array<VkSemaphore, SignalCount>            m_signalSemaphores;
	std::array<std::uint64_t, SignalCount>          m_signalValues;
	std::array<VkCommandBuffer, CommandBufferCount> m_commandBuffers;
	VkTimelineSemaphoreSubmitInfo                   m_timelineInfo;
	VkSubmitInfo                                    m_submitInfo;
	size_t                                          m_currentWaitIndex;
	size_t                                          m_currentSignalIndex;
	size_t                                          m_currentCmdBufferIndex;

public:
	QueueSubmitBuilder(const QueueSubmitBuilder& other) noexcept
		: m_waitSemaphores{ other.m_waitSemaphores },
		m_waitStages{ other.m_waitStages },
		m_waitValues{ other.m_waitValues },
		m_signalSemaphores{ other.m_signalSemaphores },
		m_signalValues{ other.m_signalValues },
		m_commandBuffers{ other.m_commandBuffers },
		m_timelineInfo{ other.m_timelineInfo },
		m_submitInfo{ other.m_submitInfo },
		m_currentWaitIndex{ other.m_currentWaitIndex },
		m_currentSignalIndex{ other.m_currentSignalIndex },
		m_currentCmdBufferIndex{ other.m_currentCmdBufferIndex }
	{
		UpdatePointers();
	}
	QueueSubmitBuilder& operator=(const QueueSubmitBuilder& other) noexcept
	{
		m_waitSemaphores        = other.m_waitSemaphores;
		m_waitStages            = other.m_waitStages;
		m_waitValues            = other.m_waitValues;
		m_signalSemaphores      = other.m_signalSemaphores;
		m_signalValues          = other.m_signalValues;
		m_commandBuffers        = other.m_commandBuffers;
		m_timelineInfo          = other.m_timelineInfo;
		m_submitInfo            = other.m_submitInfo;
		m_currentWaitIndex      = other.m_currentWaitIndex;
		m_currentSignalIndex    = other.m_currentSignalIndex;
		m_currentCmdBufferIndex = other.m_currentCmdBufferIndex;

		UpdatePointers();

		return *this;
	}
	// There is no need for moving as std::array is just a C style array. Just defining these
	// to update the pointers upon copying.
	QueueSubmitBuilder(QueueSubmitBuilder&& other) noexcept : QueueSubmitBuilder{ other } {}
	QueueSubmitBuilder& operator=(QueueSubmitBuilder&& other) noexcept { return operator=(other); }
};

class VkCommandQueue
{
public:
	VkCommandQueue(VkDevice device, VkQueue queue, std::uint32_t queueIndex)
		: m_commandQueue{ queue }, m_device{ device }, m_commandPool{ VK_NULL_HANDLE },
		m_queueIndex{ queueIndex }, m_commandBuffers{}
	{}
	~VkCommandQueue() noexcept;

	virtual void CreateCommandBuffers(std::uint32_t bufferCount);

	template<std::uint32_t WaitCount, std::uint32_t SignalCount, std::uint32_t CommandBufferCount = 1u>
	void SubmitCommandBuffer(
		const QueueSubmitBuilder<WaitCount, SignalCount, CommandBufferCount>& builder
	) const noexcept {
		vkQueueSubmit(m_commandQueue, 1u, builder.GetPtr(), VK_NULL_HANDLE);
	}

	template<std::uint32_t WaitCount = 0u, std::uint32_t SignalCount = 0u>
	void SubmitCommandBuffer(
		size_t bufferIndex, QueueSubmitBuilder<WaitCount, SignalCount>&& builder = {}
	) const noexcept {
		SubmitCommandBuffer<WaitCount, SignalCount>(
			builder.CommandBuffer(GetCommandBuffer(bufferIndex))
		);
	}

	template<std::uint32_t WaitCount, std::uint32_t SignalCount, std::uint32_t CommandBufferCount = 1u>
	void SubmitCommandBuffer(
		const QueueSubmitBuilder<WaitCount, SignalCount, CommandBufferCount>& builder,
		const VKFence& fence
	) const noexcept {
		vkQueueSubmit(m_commandQueue, 1u, builder.GetPtr(), fence.Get());
	}

	template<std::uint32_t WaitCount = 0u, std::uint32_t SignalCount = 0u>
	void SubmitCommandBuffer(
		size_t bufferIndex, const VKFence& fence,
		QueueSubmitBuilder<WaitCount, SignalCount>&& builder = {}
	) const noexcept {
		SubmitCommandBuffer<WaitCount, SignalCount>(
			builder.CommandBuffer(GetCommandBuffer(bufferIndex)), fence
		);
	}

	[[nodiscard]]
	std::uint32_t GetFamilyIndex() const noexcept { return m_queueIndex; }
	[[nodiscard]]
	VKCommandBuffer& GetCommandBuffer(size_t index) noexcept { return m_commandBuffers[index]; }
	[[nodiscard]]
	const VKCommandBuffer& GetCommandBuffer(size_t index) const noexcept
	{
		return m_commandBuffers[index];
	}
	[[nodiscard]]
	VkQueue GetQueue() const noexcept { return m_commandQueue; }

private:
	void SelfDestruct() noexcept;

protected:
	VkQueue                      m_commandQueue;
	VkDevice                     m_device;
	VkCommandPool                m_commandPool;
	std::uint32_t                m_queueIndex;
	std::vector<VKCommandBuffer> m_commandBuffers;

public:
	VkCommandQueue(const VkCommandQueue&) = delete;
	VkCommandQueue& operator=(const VkCommandQueue&) = delete;

	VkCommandQueue(VkCommandQueue&& other) noexcept
		: m_commandQueue{ other.m_commandQueue }, m_device{ other.m_device },
		m_commandPool{ std::exchange(other.m_commandPool, VK_NULL_HANDLE) },
		m_queueIndex{ other.m_queueIndex }, m_commandBuffers{ std::move(other.m_commandBuffers) }
	{}
	VkCommandQueue& operator=(VkCommandQueue&& other) noexcept
	{
		SelfDestruct();

		m_commandQueue   = other.m_commandQueue;
		m_device         = other.m_device;
		m_commandPool    = std::exchange(other.m_commandPool, VK_NULL_HANDLE);
		m_queueIndex     = other.m_queueIndex;
		m_commandBuffers = std::move(other.m_commandBuffers);

		return *this;
	}
};

class VkGraphicsQueue : public VkCommandQueue
{
public:
	VkGraphicsQueue(VkDevice device, VkQueue queue, std::uint32_t queueIndex)
		: VkCommandQueue{ device, queue, queueIndex }, m_fences{}
	{}

	void WaitForSubmission(size_t bufferIndex);
	void WaitForQueueToFinish();
	void CreateCommandBuffers(std::uint32_t bufferCount) override;

	[[nodiscard]]
	VKFence& GetFence(size_t index) noexcept { return m_fences[index]; }

private:
	std::vector<VKFence> m_fences;

public:
	VkGraphicsQueue(const VkGraphicsQueue&) = delete;
	VkGraphicsQueue& operator=(const VkGraphicsQueue&) = delete;

	VkGraphicsQueue(VkGraphicsQueue&& other) noexcept
		: VkCommandQueue{ std::move(other) }, m_fences{ std::move(other.m_fences) }
	{}

	VkGraphicsQueue& operator=(VkGraphicsQueue&& other) noexcept
	{
		VkCommandQueue::operator=(std::move(other));
		m_fences          = std::move(other.m_fences);

		return *this;
	}
};
}
#endif
