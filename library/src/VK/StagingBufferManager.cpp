#include <StagingBufferManager.hpp>
#include <ranges>
#include <algorithm>

StagingBufferManager& StagingBufferManager::AddTextureView(
	void const* cpuHandle, VkDeviceSize bufferSize,
	VkTextureView const* dst, const VkOffset3D& offset,
	QueueType dstQueueType, VkAccessFlagBits2 dstAccess, VkPipelineStageFlags2 dstStage,
	std::uint32_t mipLevelIndex/* = 0u */
) {
	m_textureData.emplace_back(
		TextureData{
			cpuHandle, bufferSize, dst, offset, mipLevelIndex,
			dstQueueType, dstAccess, dstStage
		}
	);

	Buffer& tempBuffer = m_tempBufferToTexture.emplace_back(
		m_device, m_memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	tempBuffer.Create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {});

	return *this;
}

StagingBufferManager& StagingBufferManager::AddBuffer(
	void const* cpuHandle, VkDeviceSize bufferSize, Buffer const* dst, VkDeviceSize offset,
	QueueType dstQueueType, VkAccessFlagBits2 dstAccess, VkPipelineStageFlags2 dstStage
) {
	m_bufferData.emplace_back(
		BufferData{ cpuHandle, bufferSize, dst, offset, dstQueueType, dstAccess, dstStage }
	);

	Buffer& tempBuffer = m_tempBufferToBuffer.emplace_back(
		m_device, m_memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	tempBuffer.Create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {});

	return *this;
}

void StagingBufferManager::CopyCPU()
{
	struct Batch
	{
		size_t startIndex;
		size_t endIndex;
	};

	// Making these static, so dynamic allocation happens less.
	static std::vector<std::future<void>>     waitObjs{};
	static std::vector<std::function<void()>> tasks{};
	static std::vector<Batch>                 batches{};

	static constexpr size_t batchSize = 250_MB;

	Batch tempBatch{ .startIndex = 0u, .endIndex = 0u };
	size_t currentBatchSize = 0u;

	{
		for (size_t index = 0u; index < std::size(m_bufferData); ++index)
		{
			const BufferData& bufferData = m_bufferData.at(index);

			tasks.emplace_back([&bufferData, &tempBuffer = m_tempBufferToBuffer.at(index)]
				{
					memcpy(tempBuffer.CPUHandle(), bufferData.cpuHandle, bufferData.bufferSize);
				});

			currentBatchSize += bufferData.bufferSize;

			if (currentBatchSize >= batchSize)
			{
				currentBatchSize   = 0u;

				tempBatch.endIndex = index;
				batches.emplace_back(tempBatch);

				tempBatch.startIndex = index + 1u;
				tempBatch.endIndex   = tempBatch.startIndex;
			}
		}

		// No need to finish the batching here since we might be able to put some textures into it.
	}

	{
		// Need an offset here, since I want to put both buffers and textures into the same batch.
		const size_t offset = std::size(m_bufferData);

		for (size_t index = 0u; index < std::size(m_textureData); ++index)
		{
			const TextureData& textureData = m_textureData.at(index);

			tasks.emplace_back([&textureData, &tempBuffer = m_tempBufferToTexture.at(index)]
				{
					memcpy(tempBuffer.CPUHandle(), textureData.cpuHandle, textureData.bufferSize);
				});

			currentBatchSize += textureData.bufferSize;

			if (currentBatchSize >= batchSize)
			{
				currentBatchSize   = 0u;

				tempBatch.endIndex = offset + index;
				batches.emplace_back(tempBatch);

				tempBatch.startIndex = offset + index + 1u;
				tempBatch.endIndex   = tempBatch.startIndex;
			}
		}
	}

	// Add the last element in the batch.
	if (auto elementCount = std::size(m_textureData) + std::size(m_bufferData);
		elementCount > tempBatch.endIndex)
	{
		tempBatch.endIndex = elementCount - 1u;
		batches.emplace_back(tempBatch);
	}

	// Copy batches.
	// This is asynchronous but it should be fine as different threads shouldn't access the same
	// location.
	for (auto& batch : batches)
	{
		waitObjs.emplace_back(m_threadPool->SubmitWork(std::function{
			[&tTasks = tasks, batch]
			{
				for (size_t index = batch.startIndex; index <= batch.endIndex; ++index)
				{
					tTasks.at(index)();
				}
			}}));
	}

	// Wait for all the copying to finish.
	for (auto& waitObj : waitObjs)
		waitObj.wait();

	// Only clearing, so we might not need to do dynamic allocate the next time.
	waitObjs.clear();
	tasks.clear();
	batches.clear();
}

void StagingBufferManager::CopyGPU(size_t currentCmdBufferIndex)
{
	// Assuming the command buffer has been reset before this.
	VKCommandBuffer& copyCmdBuffer = m_copyQueue->GetCommandBuffer(currentCmdBufferIndex);

	for(size_t index = 0u; index < std::size(m_bufferData); ++index)
	{
		const BufferData& bufferData = m_bufferData.at(index);
		const Buffer& tempBuffer     = m_tempBufferToBuffer.at(index);

		BufferToBufferCopyBuilder bufferBuilder = BufferToBufferCopyBuilder{}
		.Size(bufferData.bufferSize).DstOffset(bufferData.offset);

		// I am making a new buffer for each copy but if the buffer alignment for example is
		// 16 bytes and the buffer size is 4bytes, copyWhole would be wrong as it would go over
		// the reserved size in the destination buffer.
		copyCmdBuffer.Copy(tempBuffer, *bufferData.dst, bufferBuilder);
	}

	for(size_t index = 0u; index < std::size(m_textureData); ++index)
	{
		const TextureData& textureData = m_textureData.at(index);
		const Buffer& tempBuffer       = m_tempBufferToTexture.at(index);

		BufferToImageCopyBuilder bufferBuilder = BufferToImageCopyBuilder{}
		.ImageOffset(textureData.offset).ImageMipLevel(textureData.mipLevelIndex);

		// CopyWhole would not be a problem for textures, as the destination buffer would be a texture
		// and will be using the dimension of the texture instead of its size to copy. And there should
		// be only a single texture in a texture buffer.
		copyCmdBuffer.CopyWhole(tempBuffer, *textureData.dst, bufferBuilder);
	}
}

void StagingBufferManager::Copy(size_t currentCmdBufferIndex)
{
	// Since these are first copied to temp buffers and those are
	// copied on the GPU, we don't need any cpu synchronisation.
	// But we should wait on some semaphores from other queues which
	// are already running before we submit these copy commands.
	if (!std::empty(m_textureData) || !std::empty(m_bufferData))
	{
		CopyCPU();
		CopyGPU(currentCmdBufferIndex);
	}
}

void StagingBufferManager::CleanUpTempBuffers()
{
	// These need to be cleared when the copy queue is finished.
	m_tempBufferToBuffer.clear();
	m_tempBufferToTexture.clear();
}

void StagingBufferManager::CleanUpTempData()
{
	m_bufferData.clear();
	m_textureData.clear();
}

void StagingBufferManager::ReleaseOwnership(size_t currentSrcCmdBufferIndex)
{
	// If the dstQueue is not None, that should mean the resource has exclusive ownership
	// and needs an ownership transfer. The reason I have two separate functions is because
	// the acquire and release commands need to be executed on different queues, which won't
	// happen at the same time.
	VKCommandBuffer& copyCmdBuffer          = m_copyQueue->GetCommandBuffer(currentSrcCmdBufferIndex);
	const std::uint32_t transferFamilyIndex = m_copyQueue->GetFamilyIndex();

	for (const auto& bufferData : m_bufferData)
		if (bufferData.dstQueueType != QueueType::None)
		{
			const std::uint32_t dstFamilyIndex = m_queueFamilyManager->GetIndex(
				bufferData.dstQueueType
			);

			if (dstFamilyIndex != transferFamilyIndex)
				copyCmdBuffer.ReleaseOwnership(*bufferData.dst, transferFamilyIndex, dstFamilyIndex);
		}

	for (const auto& textureData : m_textureData)
		if (textureData.dstQueueType != QueueType::None)
		{
			const std::uint32_t dstFamilyIndex = m_queueFamilyManager->GetIndex(
				textureData.dstQueueType
			);

			if (dstFamilyIndex != transferFamilyIndex)
				copyCmdBuffer.ReleaseOwnership(*textureData.dst, transferFamilyIndex, dstFamilyIndex);
		}
}

void StagingBufferManager::AcquireOwnership(
	size_t currentDstCmdBufferIndex, VkCommandQueue& dstCmdQueue
) {
	// This function would most likely be called after the Release function. So, gonna erase the
	// data once they are recorded for this command.
	VKCommandBuffer& dstCmdBuffer             = dstCmdQueue.GetCommandBuffer(currentDstCmdBufferIndex);
	const std::uint32_t transferFamilyIndex   = m_copyQueue->GetFamilyIndex();
	const std::uint32_t currentDstFamilyIndex = dstCmdQueue.GetFamilyIndex();

	// Erase_if shouldn't reduce the capacity.
	std::erase_if(
		m_bufferData, [&dstCmdBuffer, transferFamilyIndex, currentDstFamilyIndex,
		queueFamilyManager = m_queueFamilyManager]
		(const BufferData& bufferData)
		{
			const std::uint32_t dstFamilyIndex = queueFamilyManager->GetIndex(
				bufferData.dstQueueType
			);
			if (currentDstFamilyIndex != dstFamilyIndex && dstFamilyIndex != transferFamilyIndex)
			{
				dstCmdBuffer.AcquireOwnership(
					*bufferData.dst, transferFamilyIndex, dstFamilyIndex,
					bufferData.dstAccess, bufferData.dstStage
				);

				return true;
			}

			return false;
		}
	);

	std::erase_if(
		m_textureData, [&dstCmdBuffer, transferFamilyIndex, currentDstFamilyIndex,
		queueFamilyManager = m_queueFamilyManager]
		(const TextureData& textureData)
		{
			const std::uint32_t dstFamilyIndex = queueFamilyManager->GetIndex(
				textureData.dstQueueType
			);
			if (currentDstFamilyIndex != dstFamilyIndex && dstFamilyIndex != transferFamilyIndex)
			{
				dstCmdBuffer.AcquireOwnership(
					*textureData.dst, transferFamilyIndex, dstFamilyIndex,
					textureData.dstAccess, textureData.dstStage
				);

				return true;
			}

			return false;
		}
	);
}
