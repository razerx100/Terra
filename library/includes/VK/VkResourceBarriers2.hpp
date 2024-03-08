#ifndef VK_RESOURCE_BARRIERS_2_HPP_
#define VK_RESOURCE_BARRIERS_2_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <cassert>
#include <VkTextureView.hpp>

template<typename VkBarrierType>
class BaseBarrierBuilder
{
public:
	BaseBarrierBuilder(VkStructureType type)
		: m_barrier{
			.sType               = type,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		} {}

	[[nodiscard]]
	VkBarrierType Get() const noexcept { return m_barrier; }

protected:
	void _queueIndices(std::uint32_t srcIndex, std::uint32_t dstIndex) noexcept
	{
		m_barrier.srcQueueFamilyIndex = srcIndex;
		m_barrier.dstQueueFamilyIndex = dstIndex;
	}

	void _accessMasks(VkAccessFlagBits2 src, VkAccessFlagBits2 dst) noexcept
	{
		m_barrier.srcAccessMask = src;
		m_barrier.dstAccessMask = dst;
	}

	void _stageMasks(VkPipelineStageFlagBits2 src, VkPipelineStageFlagBits2 dst) noexcept
	{
		m_barrier.srcStageMask = src;
		m_barrier.dstStageMask = dst;
	}

protected:
	VkBarrierType m_barrier;
};

class BufferBarrierBuilder : public BaseBarrierBuilder<VkBufferMemoryBarrier2>
{
public:
	BufferBarrierBuilder() : BaseBarrierBuilder{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 } {};

	BufferBarrierBuilder& Buffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset) noexcept
	{
		m_barrier.buffer = buffer;
		m_barrier.size   = size;
		m_barrier.offset = offset;

		return *this;
	}

	BufferBarrierBuilder& QueueIndices(std::uint32_t srcIndex, std::uint32_t dstIndex) noexcept
	{
		_queueIndices(srcIndex, dstIndex);

		return *this;
	}

	BufferBarrierBuilder& AccessMasks(VkAccessFlagBits2 src, VkAccessFlagBits2 dst) noexcept
	{
		_accessMasks(src, dst);

		return *this;
	}

	BufferBarrierBuilder& StageMasks(
		VkPipelineStageFlagBits2 src, VkPipelineStageFlagBits2 dst
	) noexcept {
		_stageMasks(src, dst);

		return *this;
	}
};

template<std::uint32_t barrierCount = 1u>
class VkBufferBarrier2
{
public:
	VkBufferBarrier2() noexcept : m_currentIndex{ 0u }, m_barriers{} {}

	void RecordBarriers(VkCommandBuffer commandBuffer) const noexcept
	{
		VkDependencyInfo dependencyInfo{
			.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.dependencyFlags          = 0u,
			.bufferMemoryBarrierCount = barrierCount,
			.pBufferMemoryBarriers    = std::data(m_barriers)
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
};

class ImageBarrierBuilder : public BaseBarrierBuilder<VkImageMemoryBarrier2>
{
public:
	ImageBarrierBuilder() : BaseBarrierBuilder{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 } {};

	ImageBarrierBuilder& QueueIndices(std::uint32_t srcIndex, std::uint32_t dstIndex) noexcept
	{
		_queueIndices(srcIndex, dstIndex);

		return *this;
	}

	ImageBarrierBuilder& AccessMasks(VkAccessFlagBits2 src, VkAccessFlagBits2 dst) noexcept
	{
		_accessMasks(src, dst);

		return *this;
	}

	ImageBarrierBuilder& StageMasks(
		VkPipelineStageFlagBits2 src, VkPipelineStageFlagBits2 dst
	) noexcept {
		_stageMasks(src, dst);

		return *this;
	}

	ImageBarrierBuilder& Image(
		VkImage image, VkImageAspectFlags imageAspect, std::uint32_t mipBaseLevel = 0u,
		std::uint32_t levelCount = 1u
	) noexcept {
		m_barrier.image = image;

		VkImageSubresourceRange& subresourceRange = m_barrier.subresourceRange;
		subresourceRange.aspectMask     = imageAspect;
		subresourceRange.baseMipLevel   = mipBaseLevel;
		subresourceRange.levelCount     = levelCount;
		subresourceRange.baseArrayLayer = 0u;
		subresourceRange.layerCount     = 1u;

		return *this;
	}

	ImageBarrierBuilder& Image(const VkTextureView& textureView) noexcept {
		return Image(
			textureView.GetTexture().Get(), textureView.GetAspect(), textureView.GetMipBaseLevel(),
			textureView.GetMipLevelCount()
		);
	}

	ImageBarrierBuilder& Layouts(VkImageLayout oldLayout, VkImageLayout newLayout) noexcept
	{
		m_barrier.oldLayout = oldLayout;
		m_barrier.newLayout = newLayout;

		return *this;
	}
};

template<std::uint32_t barrierCount = 1u>
class VkImageBarrier2
{
public:
	VkImageBarrier2() noexcept : m_currentIndex{ 0u }, m_barriers{} {}

	void RecordBarriers(VkCommandBuffer commandBuffer) const noexcept
	{
		VkDependencyInfo dependencyInfo{
			.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.dependencyFlags         = 0u,
			.imageMemoryBarrierCount = barrierCount,
			.pImageMemoryBarriers    = std::data(m_barriers)
		};

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	[[nodiscard]]
	VkImageBarrier2& AddMemoryBarrier(const VkImageMemoryBarrier2& barrier)
	{
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

	[[nodiscard]]
	VkImageBarrier2& AddMemoryBarrier(const ImageBarrierBuilder& barrier)
	{
		return AddMemoryBarrier(barrier.Get());
	}

private:
	size_t                                          m_currentIndex;
	std::array<VkImageMemoryBarrier2, barrierCount> m_barriers;
};
#endif
