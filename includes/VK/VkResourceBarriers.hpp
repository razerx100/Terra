#ifndef VK_RESOURCE_BARRIERS_HPP_
#define VK_RESOURCE_BARRIERS_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <cassert>

template<std::uint32_t barrierCount = 1u>
class VkBufferBarrier{
public:
	VkBufferBarrier() noexcept : m_currentIndex{ 0u } {}

	void RecordBarriers(VkCommandBuffer commandBuffer) noexcept {
		VkDependencyInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		info.bufferMemoryBarrierCount = barrierCount;
		info.pBufferMemoryBarriers = std::data(m_barriers);

		vkCmdPipelineBarrier2(commandBuffer, &info);
	}

	[[nodiscard]]
	VkBufferBarrier& AddBarrier(
		VkBuffer buffer,
		VkDeviceSize bufferSize, VkDeviceSize bufferOffset,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkAccessFlagBits2 srcAccess, VkAccessFlagBits2 dstAccess,
		VkPipelineStageFlagBits2 srcStage, VkPipelineStageFlagBits2 dstStage
	) noexcept {
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		VkBufferMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.srcStageMask = srcStage;
		barrier.dstStageMask = dstStage;
		barrier.buffer = buffer;
		barrier.size = bufferSize;
		barrier.offset = bufferOffset;

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

private:
	size_t m_currentIndex;
	std::array<VkBufferMemoryBarrier2, barrierCount> m_barriers;
};

template<std::uint32_t barrierCount = 1u>
class VkImageBarrier {
public:
	VkImageBarrier() noexcept : m_currentIndex{ 0u } {}

	[[nodiscard]]
	VkImageBarrier& AddOwnershipBarrier(
		VkImage image, VkImageAspectFlagBits imageAspect,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkAccessFlagBits2 srcAccess, VkAccessFlagBits2 dstAccess,
		VkPipelineStageFlagBits2 srcStage, VkPipelineStageFlagBits2 dstStage,
		VkImageLayout currentLayout
	) noexcept {
		return AddBarrier(
			image, imageAspect, srcQueueFamilyIndex, dstQueueFamilyIndex,
			currentLayout, currentLayout, srcAccess, dstAccess, srcStage, dstStage
		);
	}

	[[nodiscard]]
	VkImageBarrier& AddLayoutBarrier(
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
		VkDependencyInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		info.imageMemoryBarrierCount = barrierCount;
		info.pImageMemoryBarriers = std::data(m_barriers);

		vkCmdPipelineBarrier2(commandBuffer, &info);
	}

protected:
	[[nodiscard]]
	VkImageBarrier& AddBarrier(
		VkImage image, VkImageAspectFlagBits imageAspect,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlagBits2 srcAccess, VkAccessFlagBits2 dstAccess,
		VkPipelineStageFlagBits2 srcStage, VkPipelineStageFlagBits2 dstStage
	) noexcept {
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = imageAspect;
		subresourceRange.baseMipLevel = 0u;
		subresourceRange.levelCount = 1u;
		subresourceRange.baseArrayLayer = 0u;
		subresourceRange.layerCount = 1u;

		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.srcStageMask = srcStage;
		barrier.dstStageMask = dstStage;
		barrier.image = image;
		barrier.subresourceRange = subresourceRange;

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

private:
	size_t m_currentIndex;
	std::array<VkImageMemoryBarrier2, barrierCount> m_barriers;
};
#endif
