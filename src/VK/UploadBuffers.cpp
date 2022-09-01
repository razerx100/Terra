#include <UploadBuffers.hpp>
#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

// Cpu Base Buffers

_CpuBaseBuffers::_CpuBaseBuffers() noexcept : m_memoryTypeIndex{ 0u } {}

void _CpuBaseBuffers::CreateBuffers(VkDevice device) {
	VkDeviceMemory uploadMemory = Terra::Resources::memoryManager->GetMemoryHandle(
		m_memoryTypeIndex
	);

	VkResult result;
	for (size_t index = 0u; index < std::size(m_pBuffers); ++index) {
		BufferData& bufferData = m_allocationData[index];

		vkMapMemory(
			device, uploadMemory,
			bufferData.offset, bufferData.bufferSize, 0u,
			reinterpret_cast<void**>(&bufferData.cpuWritePtr)
		);

		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_pBuffers[index]->GetBuffer(), uploadMemory, bufferData.offset
			)
		);
	}

	AfterCreationStuff();
}

void _CpuBaseBuffers::FlushMemory(VkDevice device) {
	VkDeviceMemory uploadMemory = Terra::Resources::memoryManager->GetMemoryHandle(
		m_memoryTypeIndex
	);

	std::vector<VkMappedMemoryRange> memoryRanges;

	VkMappedMemoryRange memoryRange{};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = uploadMemory;

	for (size_t index = 0u; index < std::size(m_allocationData); ++index) {
		const BufferData& bufferData = m_allocationData[index];

		memoryRange.offset = bufferData.offset;
		memoryRange.size = bufferData.bufferSize;

		memoryRanges.emplace_back(memoryRange);
	}

	VkResult result;
	VK_THROW_FAILED(result,
		vkFlushMappedMemoryRanges(
			device, static_cast<std::uint32_t>(std::size(memoryRanges)),
			std::data(memoryRanges)
		)
	);
}

void _CpuBaseBuffers::AfterCreationStuff() {}

// Upload Buffers

void UploadBuffers::CopyData() noexcept {
	for (size_t index = 0u; index < std::size(m_allocationData); ++index) {
		const BufferData& bufferData = m_allocationData[index];

		memcpy(
			bufferData.cpuWritePtr, m_dataHandles[index].get(), bufferData.bufferSize
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

	auto [memoryOffset, memoryTypeIndex] =
		Terra::Resources::memoryManager->ReserveSizeAndGetMemoryData(
			device, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);

	m_allocationData.emplace_back(bufferSize, memoryOffset, nullptr);

	if (m_memoryTypeIndex != memoryTypeIndex)
		m_memoryTypeIndex = memoryTypeIndex;

	m_pBuffers.emplace_back(std::move(uploadBuffer));

	m_dataHandles.emplace_back(std::move(dataHandles));
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

	auto [memoryOffset, memoryTypeIndex] =
		Terra::Resources::memoryManager->ReserveSizeAndGetMemoryData(
			device, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);

	if (m_memoryTypeIndex != memoryTypeIndex)
		m_memoryTypeIndex = memoryTypeIndex;

	m_allocationData.emplace_back(memoryRequirements.size, memoryOffset, nullptr);

	return hostBuffer;
}

void HostAccessibleBuffers::AfterCreationStuff() {
	for (size_t index = 0u; index < std::size(m_pBuffers); ++index)
		m_pBuffers[index]->SetCpuHandle(m_allocationData[index].cpuWritePtr);
}

void HostAccessibleBuffers::ResetBufferData() noexcept {
	m_pBuffers = std::vector<std::shared_ptr<UploadBuffer>>();

	m_allocationData = std::vector<BufferData>();
}
