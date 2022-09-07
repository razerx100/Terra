#include <ResourceBuffer.hpp>
#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

ResourceBuffer::ResourceBuffer(
	std::vector<std::uint32_t> queueFamilyIndices, VkBufferUsageFlagBits type
) : m_queueFamilyIndices{ std::move(queueFamilyIndices) }, m_resourceType{ type } {

	m_uploadBuffers = std::make_unique<UploadBuffers>();
}

void ResourceBuffer::BindMemories(VkDevice device) {
	m_uploadBuffers->BindMemories(device);

	VkDeviceMemory gpuOnlyMemory = Terra::Resources::gpuOnlyMemory->GetMemoryHandle();

	for (size_t index = 0u; index < std::size(m_gpuBuffers); ++index)
		m_gpuBuffers[index]->BindBufferToMemory(
			device, gpuOnlyMemory, m_gpuBufferData[index].offset
		);
}

std::shared_ptr<VkResourceView> ResourceBuffer::AddBuffer(
	VkDevice device, std::unique_ptr<std::uint8_t> sourceHandle, size_t bufferSize
) {
	m_uploadBuffers->AddBuffer(device, std::move(sourceHandle), bufferSize);

	auto gpuBuffer = std::make_shared<VkResourceView>(device);

	gpuBuffer->CreateResource(
		device, bufferSize, m_resourceType | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		m_queueFamilyIndices
	);

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device, gpuBuffer->GetResource(), &memoryRequirements);

	if (!Terra::Resources::gpuOnlyMemory->CheckMemoryType(memoryRequirements))
		VK_GENERIC_THROW("Memory Type doesn't match with Buffer requirements.");

	const VkDeviceSize gpuBufferOffset =
		Terra::Resources::gpuOnlyMemory->ReserveSizeAndGetOffset(memoryRequirements);

	m_gpuBufferData.emplace_back(bufferSize, gpuBufferOffset);

	m_gpuBuffers.emplace_back(gpuBuffer);

	return gpuBuffer;
}

void ResourceBuffer::CopyData() noexcept {
	m_uploadBuffers->CopyData();
}

void ResourceBuffer::RecordUpload(VkCommandBuffer copyCmdBuffer) {
	const auto& uploadBuffers = m_uploadBuffers->GetUploadBuffers();

	for (size_t index = 0u; index < std::size(m_gpuBufferData); ++index) {
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0u;
		copyRegion.dstOffset = 0u;
		copyRegion.size = m_gpuBufferData[index].bufferSize;

		vkCmdCopyBuffer(
			copyCmdBuffer, uploadBuffers[index]->GetResource(),
			m_gpuBuffers[index]->GetResource(), 1u, &copyRegion
		);
	}
}

void ResourceBuffer::ReleaseUploadBuffer() noexcept {
	m_gpuBuffers = std::vector<std::shared_ptr<VkResourceView>>();

	m_uploadBuffers.reset();
}
