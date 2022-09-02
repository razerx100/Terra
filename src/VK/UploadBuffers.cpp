#include <UploadBuffers.hpp>
#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

// Cpu Base Buffers

void _CpuBaseBuffers::_bindMemories(VkDevice device, VkDeviceMemory memoryStart) {
	VkResult result;
	for (size_t index = 0u; index < std::size(m_pBuffers); ++index) {
		BufferData& bufferData = m_allocationData[index];

		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_pBuffers[index]->GetBuffer(), memoryStart, bufferData.offset
			)
		);
	}
}

// Upload Buffers

void UploadBuffers::CopyData() noexcept {
	std::uint8_t* cpuPtrStart = Terra::Resources::uploadMemory->GetMappedCPUPtr();

	for (size_t index = 0u; index < std::size(m_allocationData); ++index) {
		const BufferData& bufferData = m_allocationData[index];

		memcpy(
			cpuPtrStart + bufferData.offset, m_dataHandles[index].get(), bufferData.bufferSize
		);
	}
}

void UploadBuffers::AddBuffer(
	VkDevice device, std::unique_ptr<std::uint8_t> dataHandles, size_t bufferSize
) {
	std::shared_ptr<UploadBuffer> uploadBuffer = std::make_shared<UploadBuffer>(device);

	uploadBuffer->CreateBuffer(device, bufferSize);

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(device, uploadBuffer->GetBuffer(), &memoryRequirements);

	const VkDeviceSize memoryOffset =
		Terra::Resources::uploadMemory->ReserveSizeAndGetOffset(memoryRequirements);

	m_allocationData.emplace_back(bufferSize, memoryOffset);

	m_pBuffers.emplace_back(std::move(uploadBuffer));

	m_dataHandles.emplace_back(std::move(dataHandles));
}

void UploadBuffers::BindMemories(VkDevice device) {
	VkDeviceMemory uploadMemory = Terra::Resources::uploadMemory->GetMemoryHandle();

	_bindMemories(device, uploadMemory);
}

const std::vector<std::shared_ptr<UploadBuffer>>& UploadBuffers::GetUploadBuffers(
) const noexcept {
	return m_pBuffers;
}

// Host accessible Buffers
std::shared_ptr<UploadBuffer> HostAccessibleBuffers::AddBuffer(
	VkDevice device, size_t bufferSize
) {
	std::shared_ptr<UploadBuffer> hostBuffer = std::make_shared<UploadBuffer>(device);

	hostBuffer->CreateBuffer(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

	m_pBuffers.emplace_back(hostBuffer);

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device, hostBuffer->GetBuffer(), &memoryRequirements);

	const VkDeviceSize memoryOffset =
		Terra::Resources::cpuWriteMemory->ReserveSizeAndGetOffset(memoryRequirements);

	m_allocationData.emplace_back(memoryRequirements.size, memoryOffset);

	return hostBuffer;
}

void HostAccessibleBuffers::BindMemories(VkDevice device) {
	VkDeviceMemory cpuWriteMemory = Terra::Resources::cpuWriteMemory->GetMemoryHandle();

	_bindMemories(device, cpuWriteMemory);

	std::uint8_t* cpuPtrStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();

	for (size_t index = 0u; index < std::size(m_pBuffers); ++index)
		m_pBuffers[index]->SetCpuHandle(cpuPtrStart + m_allocationData[index].offset);
}

void HostAccessibleBuffers::ResetBufferData() noexcept {
	m_pBuffers = std::vector<std::shared_ptr<UploadBuffer>>();

	m_allocationData = std::vector<BufferData>();
}
