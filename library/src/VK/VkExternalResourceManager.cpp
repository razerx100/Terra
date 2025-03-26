#include <VkExternalResourceManager.hpp>
#include <limits>

VkExternalResourceManager::VkExternalResourceManager(VkDevice device, MemoryManager* memoryManager)
	: m_resourceFactory{ std::make_unique<VkExternalResourceFactory>(device, memoryManager) },
	m_gfxExtensions{}, m_copyQueueDetails{}
{}

void VkExternalResourceManager::OnGfxExtensionDeletion(const GraphicsTechniqueExtension& gfxExtension)
{
	const std::vector<std::uint32_t>& externalIndices = gfxExtension.GetExternalBufferIndices();

	for (std::uint32_t externalIndex : externalIndices)
		m_resourceFactory->RemoveExternalBuffer(externalIndex);
}

std::uint32_t VkExternalResourceManager::AddGraphicsTechniqueExtension(
	std::shared_ptr<GraphicsTechniqueExtension> extension
) {
	return static_cast<std::uint32_t>(m_gfxExtensions.Add(std::move(extension)));
}

void VkExternalResourceManager::RemoveGraphicsTechniqueExtension(std::uint32_t index) noexcept
{
	OnGfxExtensionDeletion(*m_gfxExtensions[index]);

	m_gfxExtensions.RemoveElement(index);
}

void VkExternalResourceManager::UpdateExtensionData(size_t frameIndex) const noexcept
{
	for (const GfxExtension_t& extension : m_gfxExtensions)
		extension->UpdateCPUData(frameIndex);
}

void VkExternalResourceManager::SetGraphicsDescriptorLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) {
	const size_t descriptorBufferCount = std::size(descriptorBuffers);

	for (const GfxExtension_t& extension : m_gfxExtensions)
	{
		const std::vector<ExternalBufferBindingDetails>& bindingDetails = extension->GetBindingDetails();

		for (const ExternalBufferBindingDetails& details : bindingDetails)
		{
			for (size_t index = 0u; index < descriptorBufferCount; ++index)
			{
				VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];

				if (details.layoutInfo.type == ExternalBufferType::CPUVisibleUniform)
					descriptorBuffer.AddBinding(
						details.layoutInfo.bindingIndex, s_externalBufferSetLayoutIndex,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
					);
				else
					descriptorBuffer.AddBinding(
						details.layoutInfo.bindingIndex, s_externalBufferSetLayoutIndex,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
					);
			}
		}
	}
}

void VkExternalResourceManager::UpdateDescriptor(
	std::vector<VkDescriptorBuffer>& descriptorBuffers,
	const ExternalBufferBindingDetails& bindingDetails
) const {
	const size_t descriptorBufferCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < descriptorBufferCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];
		// If there is no frameIndex. Then we add the buffer to all the frames.
		const bool isSeparateFrameDescriptor
			= bindingDetails.descriptorInfo.frameIndex != std::numeric_limits<std::uint32_t>::max()
			&& bindingDetails.descriptorInfo.frameIndex != index;

		if (isSeparateFrameDescriptor)
			continue;

		UpdateDescriptor(descriptorBuffer, bindingDetails);
	}
}

void VkExternalResourceManager::UpdateDescriptor(
	VkDescriptorBuffer& descriptorBuffer, const ExternalBufferBindingDetails& bindingDetails
) const {
	const Buffer& vkBuffer = m_resourceFactory->GetVkBuffer(
		bindingDetails.descriptorInfo.externalBufferIndex
	);

	if (bindingDetails.layoutInfo.type == ExternalBufferType::CPUVisibleUniform)
		descriptorBuffer.SetUniformBufferDescriptor(
			vkBuffer, bindingDetails.layoutInfo.bindingIndex,
			s_externalBufferSetLayoutIndex, 0u,
			static_cast<VkDeviceAddress>(bindingDetails.descriptorInfo.bufferOffset),
			static_cast<VkDeviceSize>(bindingDetails.descriptorInfo.bufferSize)
		);
	else
		descriptorBuffer.SetStorageBufferDescriptor(
			vkBuffer, bindingDetails.layoutInfo.bindingIndex,
			s_externalBufferSetLayoutIndex, 0u,
			static_cast<VkDeviceAddress>(bindingDetails.descriptorInfo.bufferOffset),
			static_cast<VkDeviceSize>(bindingDetails.descriptorInfo.bufferSize)
		);
}

void VkExternalResourceManager::UploadExternalBufferGPUOnlyData(
	StagingBufferManager& stagingBufferManager, TemporaryDataBufferGPU& tempGPUBuffer,
	std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
	size_t dstBufferOffset
) const {
	stagingBufferManager.AddBuffer(
		std::move(cpuData),
		static_cast<VkDeviceSize>(srcDataSizeInBytes),
		&m_resourceFactory->GetVkBuffer(static_cast<size_t>(externalBufferIndex)),
		static_cast<VkDeviceSize>(dstBufferOffset),
		tempGPUBuffer
	);
}

void VkExternalResourceManager::QueueExternalBufferGPUCopy(
	std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
	size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes,
	TemporaryDataBufferGPU& tempGPUBuffer
) {
	auto GetSrcSize = [&resourceFactory = m_resourceFactory]
		(std::uint32_t srcIndex, size_t srcSize) -> VkDeviceSize
		{
			auto vkSrcSize = static_cast<VkDeviceSize>(srcSize);

			if (!vkSrcSize)
				vkSrcSize = resourceFactory->GetVkBuffer(srcIndex).BufferSize();

			return vkSrcSize;
		};

	m_copyQueueDetails.emplace_back(
		GPUCopyDetails
		{
			.srcIndex  = externalBufferSrcIndex,
			.dstIndex  = externalBufferDstIndex,
			.srcOffset = static_cast<VkDeviceSize>(srcBufferOffset),
			.srcSize   = GetSrcSize(externalBufferSrcIndex, srcDataSizeInBytes),
			.dstOffset = static_cast<VkDeviceSize>(dstBufferOffset)
		}
	);

	tempGPUBuffer.Add(m_resourceFactory->GetExternalBufferSP(externalBufferSrcIndex));
}

void VkExternalResourceManager::CopyQueuedBuffers(const VKCommandBuffer& transferCmdBuffer) noexcept
{
	if (!std::empty(m_copyQueueDetails))
	{
		for (const GPUCopyDetails& copyDetails : m_copyQueueDetails)
			transferCmdBuffer.Copy(
				m_resourceFactory->GetVkBuffer(copyDetails.srcIndex),
				m_resourceFactory->GetVkBuffer(copyDetails.dstIndex),
				BufferToBufferCopyBuilder{}
					.SrcOffset(copyDetails.srcOffset)
					.Size(copyDetails.srcSize)
					.DstOffset(copyDetails.dstOffset)
			);

		m_copyQueueDetails.clear();
	}
}
