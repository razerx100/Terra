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
		VkDevice device, MemoryManager* memoryManager, ThreadPool* threadPool,
		VkQueueFamilyMananger const* queueFamilyManager
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_threadPool{ threadPool }, m_queueFamilyManager{ queueFamilyManager },
		m_bufferData{}, m_tempBufferToBuffer{}, m_textureData{}, m_tempBufferToTexture{},
		m_copyRecorded{ false }
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

	void Copy(const VKCommandBuffer& transferCmdBuffer);
	// This function should be run after the Copy function. The ownership transfer is done via a
	// barrier. So, I shouldn't need any extra syncing.
	void ReleaseOwnership(
		const VKCommandBuffer& transferCmdBuffer, std::uint32_t transferFamilyIndex
	);
	// This function should be run once in the Compute Queue and once in the Graphics Queue,
	// as to acquire ownership, this command needs to be run on a queue from the owning queue family.
	// CleanUp the TempData afterwards.
	void AcquireOwnership(
		const VKCommandBuffer& ownerQueueCmdBuffer, std::uint32_t ownerQueueFamilyIndex,
		std::uint32_t transferFamilyIndex
	);
	void CleanUpTempBuffers();
	void CleanUpTempData();

	void SetCopyRecorded() noexcept { m_copyRecorded = true; }

private:
	void CopyCPU();
	void CopyGPU(const VKCommandBuffer& transferCmdBuffer);

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
	ThreadPool*                  m_threadPool;
	VkQueueFamilyMananger const* m_queueFamilyManager;
	std::vector<BufferData>      m_bufferData;
	std::vector<Buffer>          m_tempBufferToBuffer;
	std::vector<TextureData>     m_textureData;
	std::vector<Buffer>          m_tempBufferToTexture;
	bool                         m_copyRecorded;

public:
	StagingBufferManager(const StagingBufferManager&) = delete;
	StagingBufferManager& operator=(const StagingBufferManager&) = delete;

	StagingBufferManager(StagingBufferManager&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_threadPool{ other.m_threadPool },
		m_queueFamilyManager{ other.m_queueFamilyManager },
		m_bufferData{ std::move(other.m_bufferData) },
		m_tempBufferToBuffer{ std::move(other.m_tempBufferToBuffer) },
		m_textureData{ std::move(other.m_textureData) },
		m_tempBufferToTexture{ std::move(other.m_tempBufferToTexture) },
		m_copyRecorded{ other.m_copyRecorded }
	{}

	StagingBufferManager& operator=(StagingBufferManager&& other) noexcept
	{
		m_device              = other.m_device;
		m_memoryManager       = other.m_memoryManager;
		m_threadPool          = other.m_threadPool;
		m_queueFamilyManager  = other.m_queueFamilyManager;
		m_bufferData          = std::move(other.m_bufferData);
		m_tempBufferToBuffer  = std::move(other.m_tempBufferToBuffer);
		m_textureData         = std::move(other.m_textureData);
		m_tempBufferToTexture = std::move(other.m_tempBufferToTexture);
		m_copyRecorded        = other.m_copyRecorded;

		return *this;
	}
};
#endif
