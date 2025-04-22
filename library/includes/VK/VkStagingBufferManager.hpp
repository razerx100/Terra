#ifndef	VK_STAGING_BUFFER_MANAGER_HPP_
#define VK_STAGING_BUFFER_MANAGER_HPP_
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkCommandQueue.hpp>
#include <VkQueueFamilyManager.hpp>
#include <vector>
#include <ThreadPool.hpp>
#include <TemporaryDataBuffer.hpp>

namespace Terra
{
class StagingBufferManager
{
public:
	StagingBufferManager(
		VkDevice device, MemoryManager* memoryManager, ThreadPool* threadPool,
		VkQueueFamilyMananger const* queueFamilyManager
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_threadPool{ threadPool }, m_queueFamilyManager{ queueFamilyManager },
		m_bufferInfo{}, m_tempBufferToBuffer{}, m_textureInfo{}, m_tempBufferToTexture{},
		m_cpuTempBuffer{}
	{}

	// The destination info is required, when an ownership transfer is desired. Which
	// is needed when a resource has exclusive ownership.
	StagingBufferManager& AddTextureView(
		std::shared_ptr<void> cpuData, VkTextureView const* dst, const VkOffset3D& offset,
		QueueType dstQueueType, VkAccessFlagBits2 dstAccess, VkPipelineStageFlags2 dstStage,
		Callisto::TemporaryDataBufferGPU& tempDataBuffer, std::uint32_t mipLevelIndex = 0u
	);
	StagingBufferManager& AddBuffer(
		std::shared_ptr<void> cpuData, VkDeviceSize bufferSize, Buffer const* dst,
		VkDeviceSize offset, QueueType dstQueueType, VkAccessFlagBits2 dstAccess,
		VkPipelineStageFlags2 dstStage, Callisto::TemporaryDataBufferGPU& tempDataBuffer
	);
	// If a resource has shared ownership, there is no need for ownership transfer.
	StagingBufferManager& AddTextureView(
		std::shared_ptr<void> cpuData, VkTextureView const* dst, const VkOffset3D& offset,
		Callisto::TemporaryDataBufferGPU& tempDataBuffer, std::uint32_t mipLevelIndex = 0u
	) {
		return AddTextureView(
			std::move(cpuData), dst, offset, QueueType::None, VK_ACCESS_2_NONE,
			VK_PIPELINE_STAGE_2_NONE, tempDataBuffer, mipLevelIndex
		);
	}
	StagingBufferManager& AddBuffer(
		std::shared_ptr<void> cpuData, VkDeviceSize bufferSize, Buffer const* dst,
		VkDeviceSize offset, Callisto::TemporaryDataBufferGPU& tempDataBuffer
	) {
		return AddBuffer(
			std::move(cpuData), bufferSize, dst, offset, QueueType::None, VK_ACCESS_2_NONE,
			VK_PIPELINE_STAGE_2_NONE, tempDataBuffer
		);
	}

	void CopyAndClearQueuedBuffers(const VKCommandBuffer& transferCmdBuffer);
	// This function should be run after the Copy function. The ownership transfer is done via a
	// barrier. So, I shouldn't need any extra syncing.
	void ReleaseOwnership(
		const VKCommandBuffer& transferCmdBuffer, std::uint32_t transferFamilyIndex
	);
	// This function should be run once in the Compute Queue and once in the Graphics Queue,
	// as to acquire ownership, this command needs to be run on a queue from the owning queue family.
	void AcquireOwnership(
		const VKCommandBuffer& ownerQueueCmdBuffer, std::uint32_t ownerQueueFamilyIndex,
		std::uint32_t transferFamilyIndex
	);

private:
	void CopyCPU();
	void CopyGPU(const VKCommandBuffer& transferCmdBuffer);

	void CleanUpTempBuffers() noexcept;
	void CleanUpBufferInfo() noexcept;

	[[nodiscard]]
	bool CheckForDuplicateTextureViewOwnershipTransfer(
		VkTextureView const* textureView, QueueType type
	) const noexcept;
	[[nodiscard]]
	bool CheckForDuplicateBufferOwnershipTransfer(
		Buffer const* buffer, QueueType type
	) const noexcept;

private:
	struct BufferInfo
	{
		void const*           cpuHandle;
		VkDeviceSize          bufferSize;
		Buffer const*         dst;
		VkDeviceSize          offset;
		QueueType             dstQueueType;
		VkAccessFlags2        dstAccess;
		VkPipelineStageFlags2 dstStage;
	};

	struct TextureInfo
	{
		void const*           cpuHandle;
		VkDeviceSize          bufferSize;
		VkTextureView const*  dst;
		VkOffset3D            offset;
		std::uint32_t         mipLevelIndex;
		QueueType             dstQueueType;
		VkAccessFlags2        dstAccess;
		VkPipelineStageFlags2 dstStage;
	};

private:
	VkDevice                             m_device;
	MemoryManager*                       m_memoryManager;
	ThreadPool*                          m_threadPool;
	VkQueueFamilyMananger const*         m_queueFamilyManager;
	std::vector<BufferInfo>              m_bufferInfo;
	std::vector<std::shared_ptr<Buffer>> m_tempBufferToBuffer;
	std::vector<TextureInfo>             m_textureInfo;
	std::vector<std::shared_ptr<Buffer>> m_tempBufferToTexture;
	Callisto::TemporaryDataBufferCPU     m_cpuTempBuffer;

public:
	StagingBufferManager(const StagingBufferManager&) = delete;
	StagingBufferManager& operator=(const StagingBufferManager&) = delete;

	StagingBufferManager(StagingBufferManager&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_threadPool{ other.m_threadPool },
		m_queueFamilyManager{ other.m_queueFamilyManager },
		m_bufferInfo{ std::move(other.m_bufferInfo) },
		m_tempBufferToBuffer{ std::move(other.m_tempBufferToBuffer) },
		m_textureInfo{ std::move(other.m_textureInfo) },
		m_tempBufferToTexture{ std::move(other.m_tempBufferToTexture) },
		m_cpuTempBuffer{ std::move(other.m_cpuTempBuffer) }
	{}

	StagingBufferManager& operator=(StagingBufferManager&& other) noexcept
	{
		m_device              = other.m_device;
		m_memoryManager       = other.m_memoryManager;
		m_threadPool          = other.m_threadPool;
		m_queueFamilyManager  = other.m_queueFamilyManager;
		m_bufferInfo          = std::move(other.m_bufferInfo);
		m_tempBufferToBuffer  = std::move(other.m_tempBufferToBuffer);
		m_textureInfo         = std::move(other.m_textureInfo);
		m_tempBufferToTexture = std::move(other.m_tempBufferToTexture);
		m_cpuTempBuffer       = std::move(other.m_cpuTempBuffer);

		return *this;
	}
};
}
#endif
