#include <StagingBufferManager.hpp>
#include <ranges>
#include <algorithm>

StagingBufferManager& StagingBufferManager::AddTextureView(
	std::shared_ptr<void> cpuData, VkTextureView const* dst, const VkOffset3D& offset,
	QueueType dstQueueType, VkAccessFlagBits2 dstAccess, VkPipelineStageFlags2 dstStage,
	TemporaryDataBuffer& tempDataBuffer, std::uint32_t mipLevelIndex/* = 0u */
) {
	const VkDeviceSize bufferSize = dst->GetTexture().GetBufferSize();

	m_textureInfo.emplace_back(
		TextureInfo{
			cpuData.get(), bufferSize, dst, offset, mipLevelIndex, dstQueueType, dstAccess, dstStage
		}
	);

	m_cpuTempBuffer.Add(std::move(cpuData));

	auto tempBuffer = std::make_shared<Buffer>(
		m_device, m_memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	m_tempBufferToTexture.emplace_back(tempBuffer);

	tempBuffer->Create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {});

	tempDataBuffer.Add(std::move(tempBuffer));

	return *this;
}

StagingBufferManager& StagingBufferManager::AddBuffer(
	std::shared_ptr<void> cpuData, VkDeviceSize bufferSize, Buffer const* dst, VkDeviceSize offset,
	QueueType dstQueueType, VkAccessFlagBits2 dstAccess, VkPipelineStageFlags2 dstStage,
	TemporaryDataBuffer& tempDataBuffer
) {
	m_bufferInfo.emplace_back(
		BufferInfo{ cpuData.get(), bufferSize, dst, offset, dstQueueType, dstAccess, dstStage }
	);

	m_cpuTempBuffer.Add(std::move(cpuData));

	auto tempBuffer = std::make_shared<Buffer>(
		m_device, m_memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	m_tempBufferToBuffer.emplace_back(tempBuffer);

	tempBuffer->Create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {});

	tempDataBuffer.Add(std::move(tempBuffer));

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
		for (size_t index = 0u; index < std::size(m_bufferInfo); ++index)
		{
			const BufferInfo& bufferInfo = m_bufferInfo.at(index);

			tasks.emplace_back([&bufferInfo, &tempBuffer = m_tempBufferToBuffer.at(index)]
				{
					memcpy(tempBuffer->CPUHandle(), bufferInfo.cpuHandle, bufferInfo.bufferSize);
				});

			currentBatchSize += bufferInfo.bufferSize;

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
		const size_t offset = std::size(m_bufferInfo);

		for (size_t index = 0u; index < std::size(m_textureInfo); ++index)
		{
			const TextureInfo& textureInfo = m_textureInfo.at(index);

			tasks.emplace_back([&textureInfo, &tempBuffer = m_tempBufferToTexture.at(index)]
				{
					memcpy(tempBuffer->CPUHandle(), textureInfo.cpuHandle, textureInfo.bufferSize);
				});

			currentBatchSize += textureInfo.bufferSize;

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
	if (auto elementCount = std::size(m_textureInfo) + std::size(m_bufferInfo);
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
	for(size_t index = 0u; index < std::size(m_bufferInfo); ++index)
	{
		const BufferInfo& bufferInfo = m_bufferInfo.at(index);
		const Buffer& tempBuffer     = *m_tempBufferToBuffer.at(index);

		BufferToBufferCopyBuilder bufferBuilder = BufferToBufferCopyBuilder{}
			.Size(bufferInfo.bufferSize).DstOffset(bufferInfo.offset);

		// I am making a new buffer for each copy but if the buffer alignment for example is
		// 16 bytes and the buffer size is 4bytes, copyWhole would be wrong as it would go over
		// the reserved size in the destination buffer.
		transferCmdBuffer.Copy(tempBuffer, *bufferInfo.dst, bufferBuilder);
	}

	for(size_t index = 0u; index < std::size(m_textureInfo); ++index)
	{
		const TextureInfo& textureInfo = m_textureInfo.at(index);
		const Buffer& tempBuffer       = *m_tempBufferToTexture.at(index);

		BufferToImageCopyBuilder bufferBuilder = BufferToImageCopyBuilder{}
			.ImageOffset(textureInfo.offset).ImageMipLevel(textureInfo.mipLevelIndex);

		// CopyWhole would not be a problem for textures, as the destination buffer would be a texture
		// and will be using the dimension of the texture instead of its size to copy. And there should
		// be only a single texture in a texture buffer.
		transferCmdBuffer.CopyWhole(tempBuffer, *textureInfo.dst, bufferBuilder);
	}
}

void StagingBufferManager::CopyAndClear(const VKCommandBuffer& transferCmdBuffer)
{
	// Since these are first copied to temp buffers and those are
	// copied on the GPU, we don't need any cpu synchronisation.
	// But we should wait on some semaphores from other queues which
	// are already running before we submit these copy commands.
	if (!std::empty(m_textureInfo) || !std::empty(m_bufferInfo))
	{
		CopyCPU();
		CopyGPU(transferCmdBuffer);

		// Now that the cpu copying is done. We can clear the tempData.
		m_cpuTempBuffer.Clear();

		// It's okay to clean the Temp buffers up here. As we have another instance in the
		// global tempBuffer and that one should be deleted after the resources have been
		// copied on the GPU.
		CleanUpTempBuffers();
		CleanUpBufferInfo();
	}
}

void StagingBufferManager::CleanUpTempBuffers() noexcept
{
	m_tempBufferToBuffer.clear();
	m_tempBufferToTexture.clear();
}

void StagingBufferManager::CleanUpBufferInfo() noexcept
{
	// Any bufferInfo with QueueType::None would mean that resource has shared access. And doesn't
	// require any ownership transfer. So, let's remove those.
	// Also clear the BufferInfo if the dstQueue has the same family as the transfer queue.
	// As ownership transfer won't be necessary then.
	const std::uint32_t transferQueueIndex = m_queueFamilyManager->GetIndex(QueueType::TransferQueue);

	auto eraseFunction =
		[queueFamilyManager = m_queueFamilyManager, transferQueueIndex]
		<typename T> (const T & bufferInfo) noexcept
	{
		const std::uint32_t dstQueueIndex = queueFamilyManager->GetIndex(bufferInfo.dstQueueType);

		return transferQueueIndex == dstQueueIndex || bufferInfo.dstQueueType == QueueType::None;
	};

	std::erase_if(m_bufferInfo, eraseFunction);
	std::erase_if(m_textureInfo, eraseFunction);
}

void StagingBufferManager::ReleaseOwnership(
		const VKCommandBuffer& transferCmdBuffer, std::uint32_t transferFamilyIndex
) {
	// If the dstQueue is not None, that should mean the resource has exclusive ownership
	// and needs an ownership transfer. The reason I have two separate functions is because
	// the acquire and release commands need to be executed on different queues, which won't
	// happen at the same time.
	// Any bufferInfo with its dstQueue as None should have been removed by now.

	auto ReleaseFunction = [&transferCmdBuffer, transferFamilyIndex,
		queueFamilyManager = m_queueFamilyManager] <typename T> (const std::vector<T>& bufferInfo)
	{
		for (const auto& bufferDatum : bufferInfo)
		{
			const std::uint32_t dstFamilyIndex = queueFamilyManager->GetIndex(
				bufferDatum.dstQueueType
			);

			if (dstFamilyIndex != transferFamilyIndex)
			{
				if constexpr (std::is_same_v<BufferInfo, T>)
					// If it is a buffer, then also pass the bufferSize. Because it could be a
					// SharedBuffer, which might have the allocation size bigger than the
					// actual buffer size.
					transferCmdBuffer.ReleaseOwnership(
						*bufferDatum.dst, transferFamilyIndex, dstFamilyIndex
					);
				else
					transferCmdBuffer.ReleaseOwnership(
						*bufferDatum.dst, transferFamilyIndex, dstFamilyIndex
					);
			}
		}
	};

	ReleaseFunction(m_bufferInfo);
	ReleaseFunction(m_textureInfo);
}

void StagingBufferManager::AcquireOwnership(
	const VKCommandBuffer& ownerQueueCmdBuffer, std::uint32_t ownerQueueFamilyIndex,
	std::uint32_t transferFamilyIndex
) {
	// This function would most likely be called after the Release function. So, gonna erase the
	// data once they are recorded for this command.

	auto eraseFunction = [&ownerQueueCmdBuffer, transferFamilyIndex, ownerQueueFamilyIndex,
		queueFamilyManager = m_queueFamilyManager] <typename T> (const T & bufferInfo) -> bool
	{
		const std::uint32_t dstFamilyIndex = queueFamilyManager->GetIndex(bufferInfo.dstQueueType);

		// Only remove the info if the dstFamilyIndex and the requested owner family index is the same.
		// And if the dstFamilyIndex isn't the transfer queue. Since, the transfer queue will do the
		// copy and transfer the ownership to either the Compute or the Graphics queue.
		// The bufferInfo container will have info about buffers which both graphics and compute
		// might want to own. So, we would only want to give the ownership of the buffer where the
		// dstFamilyIndex match the ComputeQueue to the ComputeQueue and the same to the Graphics queue.
		// Now the Family index of the Compute and Graphics queue might be the same. In that case
		// transferring the ownership to either of them will work.
		// Forgetting to call Acquire after Release will cause validation errors.
		if (ownerQueueFamilyIndex == dstFamilyIndex && dstFamilyIndex != transferFamilyIndex)
		{
			if constexpr (std::is_same_v<BufferInfo, T>)
				// If it is a buffer, then also pass the bufferSize. Because it could be a
				// SharedBuffer, which might have the allocation size bigger than the
				// actual buffer size.
				ownerQueueCmdBuffer.AcquireOwnership(
					*bufferInfo.dst, transferFamilyIndex, dstFamilyIndex,
					bufferInfo.dstAccess, bufferInfo.dstStage
				);
			else
				ownerQueueCmdBuffer.AcquireOwnership(
					*bufferInfo.dst, transferFamilyIndex, dstFamilyIndex,
					bufferInfo.dstAccess, bufferInfo.dstStage
				);

			return true;
		}

		return false;
	};

	// Erase_if shouldn't reduce the capacity.
	std::erase_if(m_bufferInfo, eraseFunction);
	std::erase_if(m_textureInfo, eraseFunction);
}
