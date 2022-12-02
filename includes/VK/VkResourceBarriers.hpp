#ifndef VK_RESOURCE_BARRIERS_HPP_
#define VK_RESOURCE_BARRIERS_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <cassert>

template<std::uint32_t barrierCount = 1u>
class VkBufferBarrier{
public:
	VkBufferBarrier() noexcept : m_currentIndex{ 0u } {}

	void RecordBarriers(
		VkCommandBuffer commandBuffer, VkPipelineStageFlagBits sourceStage,
		VkPipelineStageFlagBits destinationStage
	) noexcept {
		vkCmdPipelineBarrier(
			commandBuffer, sourceStage, destinationStage, 0u,
			0u, nullptr, barrierCount, std::data(m_barriers), 0u, nullptr
		);
	}

	[[nodiscard]]
	VkBufferBarrier& AddBarrier(
		VkBuffer buffer,
		VkDeviceSize bufferSize, VkDeviceSize bufferOffset,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkAccessFlagBits srcAccess, VkAccessFlagBits dstAccess
	) noexcept {
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.buffer = buffer;
		barrier.size = bufferSize;
		barrier.offset = bufferOffset;

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

private:
	size_t m_currentIndex;
	std::array<VkBufferMemoryBarrier, barrierCount> m_barriers;
};

template<std::uint32_t barrierCount = 1u>
class VkImageBarrier {
public:
	VkImageBarrier() noexcept : m_currentIndex{ 0u } {}

	[[nodiscard]]
	VkImageBarrier& AddOwnershipBarrier(
		VkImage image, VkImageAspectFlagBits imageAspect,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkAccessFlagBits srcAccess, VkAccessFlagBits dstAccess
	) noexcept {
		return AddBarrier(
			image, imageAspect, srcQueueFamilyIndex, dstQueueFamilyIndex,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, srcAccess, dstAccess
		);
	}

	[[nodiscard]]
	VkImageBarrier& AddLayoutBarrier(
		VkImage image, VkImageAspectFlagBits imageAspect,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlagBits srcAccess, VkAccessFlagBits dstAccess
	) noexcept {
		return AddBarrier(
			image, imageAspect, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			oldLayout, newLayout, srcAccess, dstAccess
		);
	}

	void RecordBarriers(
		VkCommandBuffer commandBuffer, VkPipelineStageFlagBits sourceStage,
		VkPipelineStageFlagBits destinationStage
	) noexcept {
		vkCmdPipelineBarrier(
			commandBuffer, sourceStage, destinationStage, 0u,
			0u, nullptr, 0u, nullptr, barrierCount, std::data(m_barriers)
		);
	}

protected:
	[[nodiscard]]
	VkImageBarrier& AddBarrier(
		VkImage image, VkImageAspectFlagBits imageAspect,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlagBits srcAccess, VkAccessFlagBits dstAccess
	) noexcept {
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = imageAspect;
		subresourceRange.baseMipLevel = 0u;
		subresourceRange.levelCount = 1u;
		subresourceRange.baseArrayLayer = 0u;
		subresourceRange.layerCount = 1u;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.image = image;
		barrier.subresourceRange = subresourceRange;

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

private:
	size_t m_currentIndex;
	std::array<VkImageMemoryBarrier, barrierCount> m_barriers;
};
#endif
