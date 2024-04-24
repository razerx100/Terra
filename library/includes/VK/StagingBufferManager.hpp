#ifndef	STAGING_BUFFER_MANAGER_HPP_
#define STAGING_BUFFER_MANAGER_HPP_
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkCommandQueue.hpp>
#include <VkQueueFamilyManager.hpp>
#include <vector>
#include <ThreadPool.hpp>

class StagingBufferManager
{
public:
	StagingBufferManager(
		VkDevice device, MemoryManager* memoryManager, VkCommandQueue* copyQueue,
		ThreadPool* threadPool, VkQueueFamilyMananger const* queueFamilyManager
	) : m_device{ device }, m_memoryManager{ memoryManager }, m_copyQueue{ copyQueue },
		m_threadPool{ threadPool }, m_queueFamilyManager{ queueFamilyManager },
		m_bufferData{}, m_tempBufferToBuffer{}, m_textureData{}, m_tempBufferToTexture{}
	{}

	StagingBufferManager& AddTextureView(
		void const* cpuHandle, VkDeviceSize bufferSize,
		VkTextureView const* dst, const VkOffset3D& offset,
		QueueType dstQueueType, VkAccessFlagBits2 dstAccess, VkPipelineStageFlags2 dstStage,
		std::uint32_t mipLevelIndex = 0u
	);
	StagingBufferManager& AddBuffer(
		void const* cpuHandle, VkDeviceSize bufferSize, Buffer const* dst, VkDeviceSize offset,
		QueueType dstQueueType, VkAccessFlagBits2 dstAccess, VkPipelineStageFlags2 dstStage
	);
	StagingBufferManager& AddTextureView(
		void const* cpuHandle, VkDeviceSize bufferSize,
		VkTextureView const* dst, const VkOffset3D& offset, std::uint32_t mipLevelIndex = 0u
	) {
		return AddTextureView(
			cpuHandle, bufferSize, dst, offset, QueueType::None, VK_ACCESS_2_NONE,
			VK_PIPELINE_STAGE_2_NONE, mipLevelIndex
		);
	}
	StagingBufferManager& AddBuffer(
		void const* cpuHandle, VkDeviceSize bufferSize, Buffer const* dst, VkDeviceSize offset
	) {
		return AddBuffer(
			cpuHandle, bufferSize, dst, offset, QueueType::None, VK_ACCESS_2_NONE,
			VK_PIPELINE_STAGE_2_NONE
		);
	}

	void Copy(size_t currentCmdBufferIndex);
	// This function should be run after the Copy function. The ownership transfer is done via a
	// barrier. So, I shouldn't need any extra syncing.
	void ReleaseOwnership(size_t currentSrcCmdBufferIndex);
	// This function should be run once in the Compute Queue and once in the Graphics Queue,
	// as to acquire ownership, this command needs to be run on a queue from the owning queue family.
	// CleanUp the TempData afterwards.
	void AcquireOwnership(size_t currentDstCmdBufferIndex, VkCommandQueue& dstCmdQueue);
	void CleanUpTempBuffers();
	void CleanUpTempData();

private:
	void CopyCPU();
	void CopyGPU(size_t currentCmdBufferIndex);

private:
	struct BufferData
	{
		void const*           cpuHandle;
		VkDeviceSize          bufferSize;
		Buffer const*         dst;
		VkDeviceSize          offset;
		QueueType             dstQueueType;
		VkAccessFlags2        dstAccess;
		VkPipelineStageFlags2 dstStage;
	};

	struct TextureData
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
	VkDevice                     m_device;
	MemoryManager*               m_memoryManager;
	VkCommandQueue*              m_copyQueue;
	ThreadPool*                  m_threadPool;
	VkQueueFamilyMananger const* m_queueFamilyManager;
	std::vector<BufferData>      m_bufferData;
	std::vector<Buffer>          m_tempBufferToBuffer;
	std::vector<TextureData>     m_textureData;
	std::vector<Buffer>          m_tempBufferToTexture;

public:
	StagingBufferManager(const StagingBufferManager&) = delete;
	StagingBufferManager& operator=(const StagingBufferManager&) = delete;

	StagingBufferManager(StagingBufferManager&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_copyQueue{ other.m_copyQueue },
		m_threadPool{ other.m_threadPool },
		m_queueFamilyManager{ other.m_queueFamilyManager },
		m_bufferData{ std::move(other.m_bufferData) },
		m_tempBufferToBuffer{ std::move(other.m_tempBufferToBuffer) },
		m_textureData{ std::move(other.m_textureData) },
		m_tempBufferToTexture{ std::move(other.m_tempBufferToTexture) }
	{}

	StagingBufferManager& operator=(StagingBufferManager&& other) noexcept
	{
		m_device                = other.m_device;
		m_memoryManager         = other.m_memoryManager;
		m_copyQueue             = other.m_copyQueue;
		m_threadPool            = other.m_threadPool;
		m_queueFamilyManager    = other.m_queueFamilyManager;
		m_bufferData            = std::move(other.m_bufferData);
		m_tempBufferToBuffer    = std::move(other.m_tempBufferToBuffer);
		m_textureData           = std::move(other.m_textureData);
		m_tempBufferToTexture   = std::move(other.m_tempBufferToTexture);

		return *this;
	}
};
#endif
