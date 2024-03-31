#ifndef VK_COMMAND_QUEUE_HPP_
#define VK_COMMAND_QUEUE_HPP_
#include <vulkan/vulkan.hpp>
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkResourceBarriers2.hpp>
#include <VkSyncObjects.hpp>
#include <TerraEvents.hpp>
#include <SwapchainManager.hpp>
#include <array>
#include <cassert>

class BufferToBufferCopyBuilder
{
public:
	BufferToBufferCopyBuilder()
		: m_bufferCopy{
			.srcOffset = 0u,
			.dstOffset = 0u,
			.size      = 0u
		} {}

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
		m_imageCopy.imageExtent  = extent;

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

private:
	VkBufferImageCopy m_imageCopy;
};

class VkCommandQueue;

class VKCommandBuffer
{
public:
	VKCommandBuffer();
	VKCommandBuffer(VkDevice device, VkCommandPool commandPool);

	void Create(VkDevice device, VkCommandPool commandPool);

	VKCommandBuffer& Reset() noexcept;
	VKCommandBuffer& Close() noexcept;

	VKCommandBuffer& Copy(
		const Buffer& src, const Buffer& dst, const BufferToBufferCopyBuilder& builder
	) noexcept;
	VKCommandBuffer& CopyWhole(
		const Buffer& src, const Buffer& dst, BufferToBufferCopyBuilder& builder
	) noexcept;
	VKCommandBuffer& Copy(
		const Buffer& src, const VkTextureView& dst, const BufferToImageCopyBuilder& builder
	) noexcept;
	VKCommandBuffer& CopyWhole(
		const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder& builder
	) noexcept;

	VKCommandBuffer& Copy(
		const Buffer& src, const Buffer& dst, BufferToBufferCopyBuilder&& builder
	) noexcept {
		return Copy(src, dst, builder);
	}
	VKCommandBuffer& CopyWhole(
		const Buffer& src, const Buffer& dst, BufferToBufferCopyBuilder&& builder = {}
	) noexcept
	{
		return CopyWhole(src, dst, builder);
	}
	VKCommandBuffer& Copy(
		const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder&& builder
	) noexcept {
		return Copy(src, dst, builder);
	}
	VKCommandBuffer& CopyWhole(
		const Buffer& src, const VkTextureView& dst, BufferToImageCopyBuilder&& builder = {}
	) noexcept {
		return CopyWhole(src, dst, builder);
	}

	VKCommandBuffer& AcquireOwnership(
		const Buffer& buffer,
		const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
		VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage
	) noexcept;
	VKCommandBuffer& AcquireOwnership(
		const VkTextureView& textureView,
		const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
		VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED
	) noexcept;
	VKCommandBuffer& ReleaseOwnership(
		const Buffer& buffer, const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue
	) noexcept;
	VKCommandBuffer& ReleaseOwnership(
		const VkTextureView& textureView,
		const VkCommandQueue& srcQueue, const VkCommandQueue& dstQueue,
		VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED
	) noexcept;

	template<std::uint32_t BarrierCount = 1u>
	VKCommandBuffer& AddBarrier(const VkBufferBarrier2<BarrierCount>& barrier) noexcept
	{
		barrier.RecordBarriers(m_commandBuffer);

		return *this;
	}
	template<std::uint32_t BarrierCount = 1u>
	VKCommandBuffer& AddBarrier(const VkImageBarrier2<BarrierCount>& barrier) noexcept
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
		: m_commandBuffer{ other.m_commandBuffer }
	{
		other.m_commandBuffer = VK_NULL_HANDLE;
	}
	VKCommandBuffer& operator=(VKCommandBuffer&& other) noexcept
	{
		m_commandBuffer       = other.m_commandBuffer;
		other.m_commandBuffer = VK_NULL_HANDLE;

		return *this;
	}
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
		const VKSemaphore& semaphore, VkPipelineStageFlagBits pipelineStage,
		std::uint64_t waitValue = 1u
	) {
		assert(m_currentWaitIndex < WaitCount && "More Wait semaphores than the allowed amount.");

		m_waitSemaphores.at(m_currentWaitIndex) = semaphore.Get();
		m_waitStages.at(m_currentWaitIndex)     = pipelineStage;
		m_waitValues.at(m_currentWaitIndex)     = waitValue;

		++m_currentWaitIndex;

		return *this;
	}
	QueueSubmitBuilder& SignalSemaphore(const VKSemaphore& semaphore, std::uint64_t signalValue = 1u)
	{
		assert(m_currentSignalIndex < SignalCount && "More Signal semaphores than the allowed amount.");

		m_signalSemaphores.at(m_currentSignalIndex) = semaphore.Get();
		m_signalValues.at(m_currentSignalIndex)     = signalValue;

		++m_currentSignalIndex;

		return *this;
	}
	QueueSubmitBuilder& CommandBuffer(const VKCommandBuffer& commandBuffer)
	{
		assert(
			m_currentCmdBufferIndex < CommandBufferCount
			&& "More CommandBuffers than the allowed amount."
		);

		m_commandBuffers.at(m_currentCmdBufferIndex) = commandBuffer.Get();

		++m_currentCmdBufferIndex;

		return *this;
	}

	[[nodiscard]]
	const VkSubmitInfo* GetPtr() const noexcept { return &m_submitInfo; }

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
	// No need for the copy and move ctors and assign operators, since std::array is basically
	// a C style array and has no special move ctor or assign operator.
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
	VKCommandBuffer& GetCommandBuffer(size_t index) noexcept { return m_commandBuffers.at(index); }
	[[nodiscard]]
	const VKCommandBuffer& GetCommandBuffer(size_t index) const noexcept {
		return m_commandBuffers.at(index);
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
		m_commandPool{ other.m_commandPool }, m_queueIndex{ other.m_queueIndex },
		m_commandBuffers{ std::move(other.m_commandBuffers) }
	{
		other.m_commandPool = VK_NULL_HANDLE;
	}
	VkCommandQueue& operator=(VkCommandQueue&& other) noexcept
	{
		SelfDestruct();

		m_commandQueue      = other.m_commandQueue;
		m_device            = other.m_device;
		m_commandPool       = other.m_commandPool;
		m_queueIndex        = other.m_queueIndex;
		m_commandBuffers    = std::move(other.m_commandBuffers);
		other.m_commandPool = VK_NULL_HANDLE;

		return *this;
	}
};

class VkGraphicsQueue : public VkCommandQueue
{
	class Dispatchable : public ITitanDispatchable<TerraEventType>
	{
	public:
		Dispatchable(VkGraphicsQueue& graphicsQueue) : m_graphicsQueue{ graphicsQueue } {}

		void ProcessEvent(TerraEvent& terraEvent) override
		{
			if (terraEvent.GetType() == TerraEventType::InterruptGfxQueue)
			{
				m_graphicsQueue.WaitForQueueToFinish();
			}
		}

	private:
		VkGraphicsQueue& m_graphicsQueue;
	};

public:
	VkGraphicsQueue(
		VkDevice device, VkQueue queue, std::uint32_t queueIndex, TerraDispatcher* eventDispatcher
	) : VkCommandQueue{ device, queue, queueIndex }, m_eventDispatcher{ eventDispatcher }, m_fences{},
		m_interruptGfxSub{ std::make_shared<Dispatchable>(*this) }
	{
		if (m_eventDispatcher)
			m_eventDispatcher->Subscribe(TerraEventType::InterruptGfxQueue, m_interruptGfxSub);
	}

	void WaitForSubmission(size_t bufferIndex);
	void WaitForQueueToFinish();
	void CreateCommandBuffers(std::uint32_t bufferCount) override;

	template<std::uint32_t WaitCount = 0u>
	void SubmitCommandBuffer(
		size_t bufferIndex, SwapchainManager& swapchain,
		QueueSubmitBuilder<WaitCount, 1u>&& builder = {}
	) const noexcept {
		m_fences.at(bufferIndex).Reset();

		SubmitCommandBuffer<WaitCount, 1u>(
			builder.CommandBuffer(GetCommandBuffer(bufferIndex))
			// Since this semaphore is a Binary Semaphore, its value doesn't matter.
			.SignalSemaphore(swapchain.GetSwapchainWaitSemaphore(bufferIndex))
			, m_fences.at(bufferIndex)
		);
	}

private:
	TerraDispatcher*              m_eventDispatcher;
	std::vector<VKFence>          m_fences;
	std::shared_ptr<Dispatchable> m_interruptGfxSub;

public:
	VkGraphicsQueue(const VkGraphicsQueue&) = delete;
	VkGraphicsQueue& operator=(const VkGraphicsQueue&) = delete;

	VkGraphicsQueue(VkGraphicsQueue&& other) noexcept
		: VkCommandQueue{ std::move(other) }, m_eventDispatcher{ other.m_eventDispatcher },
		m_fences{ std::move(other.m_fences) }, m_interruptGfxSub{ std::move(other.m_interruptGfxSub) }
	{}
	VkGraphicsQueue& operator=(VkGraphicsQueue&& other) noexcept
	{
		VkCommandQueue::operator=(std::move(other));
		m_eventDispatcher = other.m_eventDispatcher;
		m_fences          = std::move(other.m_fences);
		m_interruptGfxSub = std::move(other.m_interruptGfxSub);

		return *this;
	}
};
#endif
