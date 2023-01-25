#ifndef VK_RESOURCE_BARRIERS_2_HPP_
#define VK_RESOURCE_BARRIERS_2_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <cassert>

template<std::uint32_t barrierCount = 1u>
class VkBufferBarrier2 {
public:
	VkBufferBarrier2() noexcept : m_currentIndex{ 0u }, m_barriers{} {}

	void RecordBarriers(VkCommandBuffer commandBuffer) noexcept {
		VkDependencyInfo dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.dependencyFlags = 0u,
			.bufferMemoryBarrierCount = barrierCount,
			.pBufferMemoryBarriers = std::data(m_barriers)
		};

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	[[nodiscard]]
	VkBufferBarrier2& AddExecutionBarrier(
		VkBuffer buffer,
		VkDeviceSize bufferSize, VkDeviceSize bufferOffset,
		VkAccessFlagBits2 srcAccess, VkAccessFlagBits2 dstAccess,
		VkPipelineStageFlagBits2 srcStage, VkPipelineStageFlagBits2 dstStage
	) noexcept {
		return AddMemoryBarrier(
			buffer, bufferSize, bufferOffset,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			srcAccess, dstAccess, srcStage, dstStage
		);
	}

	[[nodiscard]]
	VkBufferBarrier2& AddMemoryBarrier(
		VkBuffer buffer, VkDeviceSize bufferSize, VkDeviceSize bufferOffset,
		std::uint32_t srcQueueFamilyIndex, std::uint32_t dstQueueFamilyIndex,
		VkAccessFlagBits2 srcAccess, VkAccessFlagBits2 dstAccess,
		VkPipelineStageFlagBits2 srcStage, VkPipelineStageFlagBits2 dstStage
	) noexcept {
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		VkBufferMemoryBarrier2 barrier{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
			.srcStageMask = srcStage,
			.srcAccessMask = srcAccess,
			.dstStageMask = dstStage,
			.dstAccessMask = dstAccess,
			.srcQueueFamilyIndex = srcQueueFamilyIndex,
			.dstQueueFamilyIndex = dstQueueFamilyIndex,
			.buffer = buffer,
			.offset = bufferOffset,
			.size = bufferSize
		};

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

private:
	size_t m_currentIndex;
	std::array<VkBufferMemoryBarrier2, barrierCount> m_barriers;
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
