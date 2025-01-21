#include <VkExternalResourceManager.hpp>
#include <limits>

VkExternalResourceManager::VkExternalResourceManager(VkDevice device, MemoryManager* memoryManager)
	: m_resourceFactory{ device, memoryManager }, m_gfxExtensions{}
{}

void VkExternalResourceManager::OnGfxExtensionAddition(GraphicsTechniqueExtension& gfxExtension)
{
	const std::vector<ExternalBufferDetails>& bufferDetails = gfxExtension.GetBufferDetails();

	for (const ExternalBufferDetails& details : bufferDetails)
	{
		const size_t bufferIndex = m_resourceFactory.CreateExternalBuffer(details.type);

		gfxExtension.SetBuffer(
			m_resourceFactory.GetExternalBufferSP(bufferIndex), details.bufferId,
			static_cast<std::uint32_t>(bufferIndex)
		);
	}
}

void VkExternalResourceManager::AddGraphicsTechniqueExtension(
	std::shared_ptr<GraphicsTechniqueExtension> extension
) {
	OnGfxExtensionAddition(*extension);

	m_gfxExtensions.emplace_back(std::move(extension));
}

void VkExternalResourceManager::UpdateExtensionData(size_t frameIndex) const noexcept
{
	for (const GfxExtension& extension : m_gfxExtensions)
		extension->UpdateCPUData(frameIndex);
}

void VkExternalResourceManager::SetGraphicsDescriptorLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) {
	const size_t descriptorBufferCount = std::size(descriptorBuffers);

	for (const GfxExtension& extension : m_gfxExtensions)
	{
		const std::vector<ExternalBufferBindingDetails>& bindingDetails = extension->GetBindingDetails();

		for (const ExternalBufferBindingDetails& details : bindingDetails)
		{
			for (size_t index = 0u; index < descriptorBufferCount; ++index)
			{
				VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];

				if (details.type == ExternalBufferType::CPUVisibleUniform)
					descriptorBuffer.AddBinding(
						details.bindingIndex, s_externalBufferSetLayoutIndex,
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
					);
				else
					descriptorBuffer.AddBinding(
						details.bindingIndex, s_externalBufferSetLayoutIndex,
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
			= bindingDetails.frameIndex != std::numeric_limits<std::uint32_t>::max()
			&& bindingDetails.frameIndex != index;

		if (isSeparateFrameDescriptor)
			continue;

		UpdateDescriptor(descriptorBuffer, bindingDetails);
	}
}

void VkExternalResourceManager::UpdateDescriptor(
	VkDescriptorBuffer& descriptorBuffer, const ExternalBufferBindingDetails& bindingDetails
) const {
	const VkExternalBuffer& vkBuffer = m_resourceFactory.GetVkExternalBuffer(
		bindingDetails.externalBufferIndex
	);

	if (bindingDetails.type == ExternalBufferType::CPUVisibleUniform)
		descriptorBuffer.SetUniformBufferDescriptor(
			vkBuffer.GetBuffer(), bindingDetails.bindingIndex, s_externalBufferSetLayoutIndex, 0u,
			static_cast<VkDeviceAddress>(bindingDetails.bufferOffset),
			static_cast<VkDeviceSize>(bindingDetails.bufferSize)
		);
	else
		descriptorBuffer.SetStorageBufferDescriptor(
			vkBuffer.GetBuffer(), bindingDetails.bindingIndex, s_externalBufferSetLayoutIndex, 0u,
			static_cast<VkDeviceAddress>(bindingDetails.bufferOffset),
			static_cast<VkDeviceSize>(bindingDetails.bufferSize)
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
		&m_resourceFactory.GetVkExternalBuffer(
			static_cast<size_t>(externalBufferIndex)
		).GetBuffer(),
		static_cast<VkDeviceSize>(dstBufferOffset),
		tempGPUBuffer
	);
}
