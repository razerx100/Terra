#ifndef	STAGING_BUFFER_MANAGER_HPP_
#define STAGING_BUFFER_MANAGER_HPP_
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkCommandQueue.hpp>
#include <TerraEvents.hpp>
#include <vector>
#include <ThreadPool.hpp>

class StagingBufferManager
{
	class Dispatchable : public ITitanDispatchable<TerraEventType>
	{
	public:
		Dispatchable(StagingBufferManager* stagingBufferManager)
			: m_stagingBufferManager{ stagingBufferManager }
		{}

		void ProcessEvent(TerraEvent& terraEvent) override
		{
			if (terraEvent.GetType() == TerraEventType::GfxQueueExecutionFinished)
			{
				m_stagingBufferManager->CopyGPU();
			}
		}

		void UpdateReference(StagingBufferManager* stagingManager) noexcept
		{
			m_stagingBufferManager = stagingManager;
		}

	private:
		StagingBufferManager* m_stagingBufferManager;
	};

public:
	StagingBufferManager(
		VkDevice device, MemoryManager* memoryManager, TerraDispatcher* eventDispatcher,
		VkCommandQueue* copyQueue, ThreadPool* threadPool
	) : m_device{ device }, m_memoryManager{ memoryManager }, m_eventDispatcher{ eventDispatcher },
		m_copyQueue{ copyQueue }, m_currentCmdBufferIndex{ 0u }, m_threadPool{ threadPool },
		m_bufferData{}, m_tempBufferToBuffer{}, m_textureData{}, m_tempBufferToTexture{},
		m_gfxQueueFinishedSub{ std::make_shared<Dispatchable>(this) }
	{
		m_eventDispatcher->Subscribe(TerraEventType::GfxQueueExecutionFinished, m_gfxQueueFinishedSub);
	}

	StagingBufferManager& AddTextureView(
		std::uint8_t* cpuHandle, VkDeviceSize bufferSize,
		const VkTextureView& dst, const VkOffset3D& offset, std::uint32_t mipLevelIndex = 0u
	);
	StagingBufferManager& AddBuffer(
		std::uint8_t* cpuHandle, VkDeviceSize bufferSize, const Buffer& dst, VkDeviceSize offset
	);

	void Copy(size_t currentCmdBufferIndex);
	void CleanUpTempBuffers();

private:
	void CopyCPU();
	void CopyGPU();

private:
	struct BufferData
	{
		std::uint8_t* cpuHandle;
		VkDeviceSize  bufferSize;
		const Buffer& dst;
		VkDeviceSize  offset;
	};

	struct TextureData
	{
		std::uint8_t*        cpuHandle;
		VkDeviceSize         bufferSize;
		const VkTextureView& dst;
		VkOffset3D           offset;
		std::uint32_t        mipLevelIndex;
	};

private:
	VkDevice                      m_device;
	MemoryManager*                m_memoryManager;
	TerraDispatcher*              m_eventDispatcher;
	VkCommandQueue*               m_copyQueue;
	size_t                        m_currentCmdBufferIndex;
	ThreadPool*                   m_threadPool;
	std::vector<BufferData>       m_bufferData;
	std::vector<Buffer>           m_tempBufferToBuffer;
	std::vector<TextureData>      m_textureData;
	std::vector<Buffer>           m_tempBufferToTexture;
	std::shared_ptr<Dispatchable> m_gfxQueueFinishedSub;

public:
	StagingBufferManager(const StagingBufferManager&) = delete;
	StagingBufferManager& operator=(const StagingBufferManager&) = delete;

	StagingBufferManager(StagingBufferManager&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_eventDispatcher{ other.m_eventDispatcher },
		m_copyQueue{ other.m_copyQueue },
		m_currentCmdBufferIndex{ other.m_currentCmdBufferIndex },
		m_threadPool{ other.m_threadPool },
		m_bufferData{ std::move(other.m_bufferData) },
		m_tempBufferToBuffer{ std::move(other.m_tempBufferToBuffer) },
		m_textureData{ std::move(other.m_textureData) },
		m_tempBufferToTexture{ std::move(other.m_tempBufferToTexture) },
		m_gfxQueueFinishedSub{ std::move(other.m_gfxQueueFinishedSub) }
	{
		// Need to update the reference of the owner object as we are changing the object.
		m_gfxQueueFinishedSub->UpdateReference(this);
	}

	StagingBufferManager& operator=(StagingBufferManager&& other) noexcept
	{
		m_device                = other.m_device;
		m_memoryManager         = other.m_memoryManager;
		m_eventDispatcher       = other.m_eventDispatcher;
		m_copyQueue             = other.m_copyQueue;
		m_currentCmdBufferIndex = other.m_currentCmdBufferIndex;
		m_threadPool            = other.m_threadPool;
		m_bufferData            = std::move(other.m_bufferData);
		m_tempBufferToBuffer    = std::move(other.m_tempBufferToBuffer);
		m_textureData           = std::move(other.m_textureData);
		m_tempBufferToTexture   = std::move(other.m_tempBufferToTexture);
		m_gfxQueueFinishedSub   = std::move(other.m_gfxQueueFinishedSub);

		m_gfxQueueFinishedSub->UpdateReference(this);

		return *this;
	}
};
#endif
