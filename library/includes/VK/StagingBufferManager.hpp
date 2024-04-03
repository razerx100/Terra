#ifndef	STAGING_BUFFER_MANAGER_HPP_
#define STAGING_BUFFER_MANAGER_HPP_
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkCommandQueue.hpp>
#include <vector>
#include <ThreadPool.hpp>

class StagingBufferManager
{
public:
	StagingBufferManager(
		VkDevice device, MemoryManager* memoryManager, VkCommandQueue* copyQueue,
		ThreadPool* threadPool
	) : m_device{ device }, m_memoryManager{ memoryManager }, m_copyQueue{ copyQueue },
		m_threadPool{ threadPool },
		m_bufferData{}, m_tempBufferToBuffer{}, m_textureData{}, m_tempBufferToTexture{}
	{}

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
	void CopyGPU(size_t currentCmdBufferIndex);

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
	VkCommandQueue*               m_copyQueue;
	ThreadPool*                   m_threadPool;
	std::vector<BufferData>       m_bufferData;
	std::vector<Buffer>           m_tempBufferToBuffer;
	std::vector<TextureData>      m_textureData;
	std::vector<Buffer>           m_tempBufferToTexture;

public:
	StagingBufferManager(const StagingBufferManager&) = delete;
	StagingBufferManager& operator=(const StagingBufferManager&) = delete;

	StagingBufferManager(StagingBufferManager&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_copyQueue{ other.m_copyQueue },
		m_threadPool{ other.m_threadPool },
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
		m_bufferData            = std::move(other.m_bufferData);
		m_tempBufferToBuffer    = std::move(other.m_tempBufferToBuffer);
		m_textureData           = std::move(other.m_textureData);
		m_tempBufferToTexture   = std::move(other.m_tempBufferToTexture);

		return *this;
	}
};
#endif
