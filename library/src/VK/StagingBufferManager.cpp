#include <StagingBufferManager.hpp>
#include <ranges>
#include <algorithm>

StagingBufferManager& StagingBufferManager::AddTextureView(
	void const* cpuHandle, VkDeviceSize bufferSize,
	VkTextureView const* dst, const VkOffset3D& offset,
	QueueType dstQueueType, VkAccessFlagBits2 dstAccess, VkPipelineStageFlags2 dstStage,
	std::uint32_t mipLevelIndex/* = 0u */
) {
	if (m_copyRecorded)
	{
		// If this boolean is set, that would mean the temp buffers have already been recorded
		// and this function should only be called after the queues have finished. So, it should
		// be okay to clean them up.
		CleanUpTempBuffers();
		CleanUpTempData();

		m_copyRecorded = false;
	}

	m_textureData.emplace_back(
		TextureData{
			cpuHandle, bufferSize, dst, offset, mipLevelIndex, dstQueueType, dstAccess, dstStage
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
	if (m_copyRecorded)
	{
		// If this boolean is set, that would mean the temp buffers have already been recorded
		// and this function should only be called after the queues have finished. So, it should
		// be okay to clean them up.
		CleanUpTempBuffers();
		CleanUpTempData();

		m_copyRecorded = false;
	}

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

void StagingBufferManager::CopyGPU(const VKCommandBuffer& transferCmdBuffer)
{
	// Assuming the command buffer has been reset before this.
	for(size_t index = 0u; index < std::size(m_bufferData); ++index)
	{
		const BufferData& bufferData = m_bufferData.at(index);
		const Buffer& tempBuffer     = m_tempBufferToBuffer.at(index);

		BufferToBufferCopyBuilder bufferBuilder = BufferToBufferCopyBuilder{}
		.Size(bufferData.bufferSize).DstOffset(bufferData.offset);

		// I am making a new buffer for each copy but if the buffer alignment for example is
		// 16 bytes and the buffer size is 4bytes, copyWhole would be wrong as it would go over
		// the reserved size in the destination buffer.
		transferCmdBuffer.Copy(tempBuffer, *bufferData.dst, bufferBuilder);
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
		transferCmdBuffer.CopyWhole(tempBuffer, *textureData.dst, bufferBuilder);
	}
}

void StagingBufferManager::Copy(const VKCommandBuffer& transferCmdBuffer)
{
	// Since these are first copied to temp buffers and those are
	// copied on the GPU, we don't need any cpu synchronisation.
	// But we should wait on some semaphores from other queues which
	// are already running before we submit these copy commands.
	if (!std::empty(m_textureData) || !std::empty(m_bufferData))
	{
		CopyCPU();
		CopyGPU(transferCmdBuffer);

		// This should clean the temp vulkan buffers when a new buffer is added to
		// the staging manager next.
		SetCopyRecorded();
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

void StagingBufferManager::ReleaseOwnership(
		const VKCommandBuffer& transferCmdBuffer, std::uint32_t transferFamilyIndex
) {
	// If the dstQueue is not None, that should mean the resource has exclusive ownership
	// and needs an ownership transfer. The reason I have two separate functions is because
	// the acquire and release commands need to be executed on different queues, which won't
	// happen at the same time.

	auto ReleaseFunction = [&transferCmdBuffer, transferFamilyIndex,
		queueFamilyManager = m_queueFamilyManager] <typename T> (const std::vector<T>& bufferData)
	{
		for (const auto& bufferDatum : bufferData)
			if (bufferDatum.dstQueueType != QueueType::None)
			{
				const std::uint32_t dstFamilyIndex = queueFamilyManager->GetIndex(
					bufferDatum.dstQueueType
				);

				if (dstFamilyIndex != transferFamilyIndex)
				{
					if constexpr (std::is_same_v<BufferData, T>)
						// If it is a buffer, then also pass the bufferSize.
						transferCmdBuffer.ReleaseOwnership(
							*bufferDatum.dst, bufferDatum.bufferSize, transferFamilyIndex,
							dstFamilyIndex
						);
					else
						transferCmdBuffer.ReleaseOwnership(
							*bufferDatum.dst, transferFamilyIndex, dstFamilyIndex
						);
				}
			}
	};

	ReleaseFunction(m_bufferData);
	ReleaseFunction(m_textureData);
}

void StagingBufferManager::AcquireOwnership(
	const VKCommandBuffer& ownerQueueCmdBuffer, std::uint32_t ownerQueueFamilyIndex,
	std::uint32_t transferFamilyIndex
) {
	// This function would most likely be called after the Release function. So, gonna erase the
	// data once they are recorded for this command.

	auto eraseFunction = [&ownerQueueCmdBuffer, transferFamilyIndex, ownerQueueFamilyIndex,
		queueFamilyManager = m_queueFamilyManager] <typename T> (const T & bufferData) -> bool
	{
		const std::uint32_t dstFamilyIndex = queueFamilyManager->GetIndex(bufferData.dstQueueType);

		// Only remove the data if the dstFamilyIndex and the requested owner family index is the same.
		// And if the dstFamilyIndex isn't the transfer queue. Since, the transfer queue will do the
		// copy and transfer the ownership to either the Compute or the Graphics queue.
		// The bufferData container will have buffer which both graphics and compute might want to
		// own. So, we would only want to give the ownership of the data where the dstFamilyIndex
		// match the ComputeQueue to the ComputeQueue and the same to the Graphics queue.
		// Now the Family index of the Compute and Graphics queue might be the same. In that case
		// transferring the ownership to either of them will work.
		// Forgetting to call Acquire after Release will cause validation errors.
		if (ownerQueueFamilyIndex == dstFamilyIndex && dstFamilyIndex != transferFamilyIndex)
		{
			if constexpr (std::is_same_v<BufferData, T>)
				// If it is a buffer, then also pass the bufferSize.
				ownerQueueCmdBuffer.AcquireOwnership(
					*bufferData.dst, bufferData.bufferSize, transferFamilyIndex, dstFamilyIndex,
					bufferData.dstAccess, bufferData.dstStage
				);
			else
				ownerQueueCmdBuffer.AcquireOwnership(
					*bufferData.dst, transferFamilyIndex, dstFamilyIndex,
					bufferData.dstAccess, bufferData.dstStage
				);

			return true;
		}

		return false;
	};

	// Erase_if shouldn't reduce the capacity.
	std::erase_if(m_bufferData, eraseFunction);
	std::erase_if(m_textureData, eraseFunction);
}
