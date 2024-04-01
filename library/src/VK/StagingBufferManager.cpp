#include <StagingBufferManager.hpp>

StagingBufferManager& StagingBufferManager::AddTextureView(
	std::uint8_t* cpuHandle, VkDeviceSize bufferSize,
	const VkTextureView& dst, const VkOffset3D& offset, std::uint32_t mipLevelIndex/* = 0u */
) {
	m_textureData.emplace_back(TextureData{ cpuHandle, bufferSize, dst, offset, mipLevelIndex });

	Buffer& tempBuffer = m_tempBufferToTexture.emplace_back(
		m_device, m_memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	tempBuffer.Create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {});

	return *this;
}

StagingBufferManager& StagingBufferManager::AddBuffer(
	std::uint8_t* cpuHandle, VkDeviceSize bufferSize, const Buffer& dst, VkDeviceSize offset
) {
	m_bufferData.emplace_back(BufferData{cpuHandle, bufferSize, dst, offset});

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

			tasks.emplace_back([&] {
				const Buffer& tempBuffer = m_tempBufferToBuffer.at(index);

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

			tasks.emplace_back([&] {
					const Buffer& tempBuffer = m_tempBufferToTexture.at(index);

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

void StagingBufferManager::CopyGPU()
{
	// Assuming the command buffer has been reset before this.
	VKCommandBuffer& copyCmdBuffer = m_copyQueue->GetCommandBuffer(m_currentCmdBufferIndex);

	for(size_t index = 0u; index < std::size(m_bufferData); ++index)
	{
		const BufferData& bufferData = m_bufferData.at(index);
		const Buffer& tempBuffer     = m_tempBufferToBuffer.at(index);

		BufferToBufferCopyBuilder bufferBuilder = BufferToBufferCopyBuilder{}
		.DstOffset(bufferData.offset);

		copyCmdBuffer.CopyWhole(tempBuffer, bufferData.dst, bufferBuilder);
	}

	for(size_t index = 0u; index < std::size(m_bufferData); ++index)
	{
		const TextureData& textureData = m_textureData.at(index);
		const Buffer& tempBuffer       = m_tempBufferToTexture.at(index);

		BufferToImageCopyBuilder bufferBuilder = BufferToImageCopyBuilder{}
		.ImageOffset(textureData.offset).ImageMipLevel(textureData.mipLevelIndex);

		copyCmdBuffer.CopyWhole(tempBuffer, textureData.dst, bufferBuilder);
	}
}

void StagingBufferManager::Copy(size_t currentCmdBufferIndex)
{
	// Don't interrupt the Gfx Queue unless there is something to be copied.
	if (!std::empty(m_textureData) || !std::empty(m_bufferData))
	{
		CopyCPU();

		m_currentCmdBufferIndex = currentCmdBufferIndex;
		// Copy on GPU will be done in response to the GfxQueueExecutionFinished event.
		m_eventDispatcher->Dispatch(InterruptGfxQueueEvent{});

		m_bufferData.clear();
		m_tempBufferToBuffer.clear();
		m_textureData.clear();
		m_tempBufferToTexture.clear();
	}
}
