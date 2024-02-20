#ifndef VK_RESOURCE_BARRIERS_2_HPP_
#define VK_RESOURCE_BARRIERS_2_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <cassert>

class BufferBarrierBuilder
{
public:
	BufferBarrierBuilder() : m_barrier{ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 } {};

	BufferBarrierBuilder& Buffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset) noexcept
	{
		m_barrier.buffer = buffer;
		m_barrier.size   = size;
		m_barrier.offset = offset;

		return *this;
	}

	BufferBarrierBuilder& QueueIndices(std::uint32_t srcIndex, std::uint32_t dstIndex) noexcept
	{
		m_barrier.srcQueueFamilyIndex = srcIndex;
		m_barrier.dstQueueFamilyIndex = dstIndex;

		return *this;
	}

	BufferBarrierBuilder& AccessMasks(VkAccessFlagBits2 src, VkAccessFlagBits2 dst) noexcept
	{
		m_barrier.srcAccessMask = src;
		m_barrier.dstAccessMask = dst;

		return *this;
	}

	BufferBarrierBuilder& StageMasks(
		VkPipelineStageFlagBits2 src, VkPipelineStageFlagBits2 dst
	) noexcept {
		m_barrier.srcStageMask = src;
		m_barrier.dstStageMask = dst;

		return *this;
	}

	[[nodiscard]]
	VkBufferMemoryBarrier2 Get() const noexcept { return m_barrier; }

private:
	VkBufferMemoryBarrier2 m_barrier;
};

template<std::uint32_t barrierCount = 1u>
class VkBufferBarrier2 {
public:
	VkBufferBarrier2() noexcept : m_currentIndex{ 0u }, m_barriers{} {}

	void RecordBarriers(VkCommandBuffer commandBuffer) noexcept
	{
		VkDependencyInfo dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.dependencyFlags = 0u,
			.bufferMemoryBarrierCount = barrierCount,
			.pBufferMemoryBarriers = std::data(m_barriers)
		};

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	VkBufferBarrier2& AddMemoryBarrier(const VkBufferMemoryBarrier2& barrier)
	{
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}
	VkBufferBarrier2& AddMemoryBarrier(const BufferBarrierBuilder& barrier)
	{
		return AddMemoryBarrier(barrier.Get());
	}

private:
	size_t                                           m_currentIndex;
	std::array<VkBufferMemoryBarrier2, barrierCount> m_barriers;

public:
	VkBufferBarrier2(const VkBufferBarrier2& other) noexcept
		: m_currentIndex{ other.m_currentIndex }, m_barriers{ other.m_barriers } {}

	VkBufferBarrier2& operator=(const VkBufferBarrier2& other) noexcept
	{
		m_currentIndex = other.m_currentIndex;
		m_barriers     = other.m_barriers;

		return *this;
	}

	VkBufferBarrier2(VkBufferBarrier2&& other) noexcept
		: m_currentIndex{ other.m_currentIndex }, m_barriers{ std::move(other.m_barriers) } {}

	VkBufferBarrier2& operator=(VkBufferBarrier2&& other) noexcept
	{
		m_currentIndex = other.m_currentIndex;
		m_barriers     = std::move(other.m_barriers);

		return *this;
	}
};

template<std::uint32_t barrierCount = 1u>
class VkImageBarrier2 {
public:
	VkImageBarrier2() noexcept : m_currentIndex{ 0u }, m_barriers{} {}

	[[nodiscard]]
	VkImageBarrier2& AddMemoryBarrier(
		VkImage image, VkImageAspectFlagBits imageAspect,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkAccessFlagBits2 srcAccess, VkAccessFlagBits2 dstAccess, VkImageLayout currentLayout,
		VkPipelineStageFlagBits2 srcStage, VkPipelineStageFlagBits2 dstStage
	) noexcept {
		return AddBarrier(
			image, imageAspect, srcQueueFamilyIndex, dstQueueFamilyIndex,
			currentLayout, currentLayout, srcAccess, dstAccess, srcStage, dstStage
		);
	}

	[[nodiscard]]
	VkImageBarrier2& AddExecutionBarrier(
		VkImage image, VkImageAspectFlagBits imageAspect,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlagBits2 srcAccess, VkAccessFlagBits2 dstAccess,
		VkPipelineStageFlagBits2 srcStage, VkPipelineStageFlagBits2 dstStage
	) noexcept {
		return AddBarrier(
			image, imageAspect, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			oldLayout, newLayout, srcAccess, dstAccess, srcStage, dstStage
		);
	}

	void RecordBarriers(VkCommandBuffer commandBuffer) noexcept {
		VkDependencyInfo dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.dependencyFlags = 0u,
			.imageMemoryBarrierCount = barrierCount,
			.pImageMemoryBarriers = std::data(m_barriers)
		};

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

protected:
	[[nodiscard]]
	VkImageBarrier2& AddBarrier(
		VkImage image, VkImageAspectFlagBits imageAspect,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlagBits2 srcAccess, VkAccessFlagBits2 dstAccess,
		VkPipelineStageFlagBits2 srcStage, VkPipelineStageFlagBits2 dstStage
	) noexcept {
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		VkImageSubresourceRange subresourceRange{
			.aspectMask = static_cast<VkImageAspectFlags>(imageAspect),
			.baseMipLevel = 0u,
			.levelCount = 1u,
			.baseArrayLayer = 0u,
			.layerCount = 1u
		};

		VkImageMemoryBarrier2 barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = srcStage,
			.srcAccessMask = srcAccess,
			.dstStageMask = dstStage,
			.dstAccessMask = dstAccess,
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.srcQueueFamilyIndex = srcQueueFamilyIndex,
			.dstQueueFamilyIndex = dstQueueFamilyIndex,
			.image = image,
			.subresourceRange = subresourceRange
		};

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

private:
	size_t m_currentIndex;
	std::array<VkImageMemoryBarrier2, barrierCount> m_barriers;
};
#endif
