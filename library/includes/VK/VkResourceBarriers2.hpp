#ifndef VK_RESOURCE_BARRIERS_2_HPP_
#define VK_RESOURCE_BARRIERS_2_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <vector>
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
		}
	{}

	[[nodiscard]]
	const VkBarrierType& Get() const noexcept { return m_barrier; }

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

	BufferBarrierBuilder& Buffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0u) noexcept
	{
		m_barrier.buffer = buffer;
		m_barrier.size   = size;
		m_barrier.offset = offset;

		return *this;
	}

	BufferBarrierBuilder& Buffer(
		const ::Buffer& buffer, VkDeviceSize bufferSize, VkDeviceSize offset = 0u
	) noexcept {
		return Buffer(buffer.Get(), bufferSize, offset);
	}

	BufferBarrierBuilder& Buffer(const ::Buffer& buffer) noexcept
	{
		return Buffer(buffer.Get(), buffer.BufferSize(), 0u);
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

// This one should not be created and destroyed on every frame.
class VkBufferBarrier2_1
{
public:
	VkBufferBarrier2_1() : m_barriers{} {}
	VkBufferBarrier2_1(size_t totalBarrierCount) : VkBufferBarrier2_1{}
	{
		m_barriers.reserve(totalBarrierCount);
	}

	VkBufferBarrier2_1& AddMemoryBarrier(const VkBufferMemoryBarrier2& barrier)
	{
		m_barriers.emplace_back(barrier);

		return *this;
	}

	VkBufferBarrier2_1& AddMemoryBarrier(const BufferBarrierBuilder& barrier)
	{
		return AddMemoryBarrier(barrier.Get());
	}

	// These functions can be used every frame.
	void SetBuffer(
		size_t barrierIndex, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0u
	)  noexcept {
		VkBufferMemoryBarrier2& barrier = m_barriers[barrierIndex];

		barrier.buffer = buffer;
		barrier.size   = size;
		barrier.offset = offset;
	}

	void SetBuffer(
		size_t barrierIndex, const Buffer& buffer, VkDeviceSize size, VkDeviceSize offset = 0u
	) noexcept {
		SetBuffer(barrierIndex, buffer.Get(), size, offset);
	}

	void SetBuffer(size_t barrierIndex, const Buffer& buffer) noexcept
	{
		SetBuffer(barrierIndex, buffer.Get(), buffer.BufferSize(), 0u);
	}

	void RecordBarriers(VkCommandBuffer commandBuffer) const noexcept
	{
		VkDependencyInfo dependencyInfo
		{
			.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.dependencyFlags          = 0u,
			.bufferMemoryBarrierCount = static_cast<std::uint32_t>(std::size(m_barriers)),
			.pBufferMemoryBarriers    = std::data(m_barriers)
		};

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	[[nodiscard]]
	std::uint32_t GetCount() const noexcept
	{
		return static_cast<std::uint32_t>(std::size(m_barriers));
	}

private:
	std::vector<VkBufferMemoryBarrier2> m_barriers;

public:
	VkBufferBarrier2_1(const VkBufferBarrier2_1& other) noexcept
		: m_barriers{ other.m_barriers }
	{}
	VkBufferBarrier2_1& operator=(const VkBufferBarrier2_1& other) noexcept
	{
		m_barriers = other.m_barriers;

		return *this;
	}

	VkBufferBarrier2_1(VkBufferBarrier2_1&& other) noexcept
		: m_barriers{ std::move(other.m_barriers) }
	{}
	VkBufferBarrier2_1& operator=(VkBufferBarrier2_1&& other) noexcept
	{
		m_barriers = std::move(other.m_barriers);

		return *this;
	}
};

template<std::uint32_t barrierCount = 1u>
// This one can be created and destroyed on every frame.
class VkBufferBarrier2
{
public:
	VkBufferBarrier2() : m_currentIndex{ 0u }, m_barriers{} {}

	void RecordBarriers(VkCommandBuffer commandBuffer) const noexcept
	{
		VkDependencyInfo dependencyInfo
		{
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

	[[nodiscard]]
	std::uint32_t GetCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_currentIndex);
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

	ImageBarrierBuilder& Image(const VKImageView& imageView) noexcept
	{
		return Image(
			imageView.GetImage(), imageView.GetAspect(), imageView.GetMipBaseLevel(),
			imageView.GetMipLevelCount()
		);
	}

	ImageBarrierBuilder& Image(const VkTextureView& textureView) noexcept
	{
		return Image(textureView.GetView());
	}

	ImageBarrierBuilder& Layouts(VkImageLayout oldLayout, VkImageLayout newLayout) noexcept
	{
		m_barrier.oldLayout = oldLayout;
		m_barrier.newLayout = newLayout;

		return *this;
	}
};

// This one should not be created and destroyed on every frame.
class VkImageBarrier2_1
{
public:
	VkImageBarrier2_1() : m_barriers{} {}
	VkImageBarrier2_1(size_t totalBarrierCount) : VkImageBarrier2_1{}
	{
		m_barriers.reserve(totalBarrierCount);
	}

	VkImageBarrier2_1& AddMemoryBarrier(const VkImageMemoryBarrier2& barrier)
	{
		m_barriers.emplace_back(barrier);

		return *this;
	}

	VkImageBarrier2_1& AddMemoryBarrier(const ImageBarrierBuilder& barrier)
	{
		return AddMemoryBarrier(barrier.Get());
	}

	// These functions can be used every frame.
	void SetImage(
		size_t barrierIndex, VkImage image, VkImageAspectFlags imageAspect,
		std::uint32_t mipBaseLevel = 0u, std::uint32_t levelCount = 1u
	)  noexcept {
		VkImageMemoryBarrier2& barrier = m_barriers[barrierIndex];

		barrier.image = image;

		VkImageSubresourceRange& subresourceRange = barrier.subresourceRange;
		subresourceRange.aspectMask     = imageAspect;
		subresourceRange.baseMipLevel   = mipBaseLevel;
		subresourceRange.levelCount     = levelCount;
		subresourceRange.baseArrayLayer = 0u;
		subresourceRange.layerCount     = 1u;
	}

	void SetSrcStage(size_t barrierIndex, VkPipelineStageFlagBits2 srcStageFlag) noexcept
	{
		VkImageMemoryBarrier2& barrier = m_barriers[barrierIndex];

		barrier.srcStageMask = srcStageFlag;
	}

	void SetImage(size_t barrierIndex, const VKImageView& imageView) noexcept
	{
		SetImage(
			barrierIndex, imageView.GetImage(), imageView.GetAspect(), imageView.GetMipBaseLevel(),
			imageView.GetMipLevelCount()
		);
	}

	void SetImage(size_t barrierIndex, const VkTextureView& textureView) noexcept
	{
		SetImage(barrierIndex, textureView.GetView());
	}

	void RecordBarriers(VkCommandBuffer commandBuffer) const noexcept
	{
		VkDependencyInfo dependencyInfo
		{
			.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.dependencyFlags         = 0u,
			.imageMemoryBarrierCount = static_cast<std::uint32_t>(std::size(m_barriers)),
			.pImageMemoryBarriers    = std::data(m_barriers)
		};

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	[[nodiscard]]
	std::uint32_t GetCount() const noexcept
	{
		return static_cast<std::uint32_t>(std::size(m_barriers));
	}

private:
	std::vector<VkImageMemoryBarrier2> m_barriers;

public:
	VkImageBarrier2_1(const VkImageBarrier2_1& other) noexcept
		: m_barriers{ other.m_barriers }
	{}
	VkImageBarrier2_1& operator=(const VkImageBarrier2_1& other) noexcept
	{
		m_barriers = other.m_barriers;

		return *this;
	}

	VkImageBarrier2_1(VkImageBarrier2_1&& other) noexcept
		: m_barriers{ std::move(other.m_barriers) }
	{}
	VkImageBarrier2_1& operator=(VkImageBarrier2_1&& other) noexcept
	{
		m_barriers = std::move(other.m_barriers);

		return *this;
	}
};

template<std::uint32_t barrierCount = 1u>
// This one can be created and destroyed on every frame.
class VkImageBarrier2
{
public:
	VkImageBarrier2() : m_currentIndex{ 0u }, m_barriers{} {}

	void RecordBarriers(VkCommandBuffer commandBuffer) const noexcept
	{
		VkDependencyInfo dependencyInfo
		{
			.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.dependencyFlags         = 0u,
			.imageMemoryBarrierCount = barrierCount,
			.pImageMemoryBarriers    = std::data(m_barriers)
		};

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	VkImageBarrier2& AddMemoryBarrier(const VkImageMemoryBarrier2& barrier)
	{
		assert(m_currentIndex < barrierCount && "Barrier Count exceeded.");

		m_barriers[m_currentIndex] = barrier;
		++m_currentIndex;

		return *this;
	}

	VkImageBarrier2& AddMemoryBarrier(const ImageBarrierBuilder& barrier)
	{
		return AddMemoryBarrier(barrier.Get());
	}

	[[nodiscard]]
	std::uint32_t GetCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_currentIndex);
	}

private:
	size_t                                          m_currentIndex;
	std::array<VkImageMemoryBarrier2, barrierCount> m_barriers;
};
#endif
