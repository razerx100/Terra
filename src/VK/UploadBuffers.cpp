#include <UploadBuffers.hpp>
#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

// Cpu Base Buffers
void _CpuBaseBuffers::_bindMemories(VkDevice device, VkDeviceMemory memoryStart) {
	for (size_t index = 0u; index < std::size(m_pBuffers); ++index)
		m_pBuffers[index]->BindResourceToMemory(device, memoryStart);
}

// Upload Buffers
void UploadBuffers::CopyData() noexcept {
	std::uint8_t* cpuPtrStart = Terra::Resources::uploadMemory->GetMappedCPUPtr();

	for (size_t index = 0u; index < std::size(m_dataHandles); ++index) {
		memcpy(
			cpuPtrStart + m_pBuffers[index]->GetMemoryOffset(), m_dataHandles[index].get(),
			m_bufferSizes[index]
		);
	}
}

void UploadBuffers::AddBuffer(
	VkDevice device, std::unique_ptr<std::uint8_t> dataHandles, size_t bufferSize
) {
	auto uploadBuffer = std::make_shared<VkResourceView>(device);

	uploadBuffer->CreateResource(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device, uploadBuffer->GetResource(), &memoryRequirements);

	const VkDeviceSize memoryOffset =
		Terra::Resources::uploadMemory->ReserveSizeAndGetOffset(memoryRequirements);

	uploadBuffer->SetMemoryOffset(memoryOffset);
	m_bufferSizes.emplace_back(bufferSize);
	m_pBuffers.emplace_back(std::move(uploadBuffer));
	m_dataHandles.emplace_back(std::move(dataHandles));
}

void UploadBuffers::BindMemories(VkDevice device) {
	VkDeviceMemory uploadMemory = Terra::Resources::uploadMemory->GetMemoryHandle();

	_bindMemories(device, uploadMemory);
}

const std::vector<std::shared_ptr<VkResourceView>>& UploadBuffers::GetUploadBuffers(
) const noexcept {
	return m_pBuffers;
}

// Host accessible Buffers
std::shared_ptr<VkResourceView> HostAccessibleBuffers::AddBuffer(
	VkDevice device, size_t bufferSize
) {
	auto hostBuffer = std::make_shared<VkResourceView>(device);

	hostBuffer->CreateResource(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device, hostBuffer->GetResource(), &memoryRequirements);

	const VkDeviceSize memoryOffset =
		Terra::Resources::cpuWriteMemory->ReserveSizeAndGetOffset(memoryRequirements);

	hostBuffer->SetMemoryOffset(memoryOffset);
	m_pBuffers.emplace_back(hostBuffer);
	m_bufferSizes.emplace_back(memoryRequirements.size);

	return hostBuffer;
}

void HostAccessibleBuffers::BindMemories(VkDevice device) {
	VkDeviceMemory cpuWriteMemory = Terra::Resources::cpuWriteMemory->GetMemoryHandle();

	_bindMemories(device, cpuWriteMemory);
}

void HostAccessibleBuffers::ResetBufferData() noexcept {
	m_pBuffers = std::vector<std::shared_ptr<VkResourceView>>();
	m_bufferSizes = std::vector<size_t>();
}
