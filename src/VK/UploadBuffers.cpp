#include <UploadBuffers.hpp>
#include <DeviceMemory.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

// Cpu Base Buffers

_CpuBaseBuffers::_CpuBaseBuffers(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice
) : m_cpuHandle(nullptr), m_currentOffset(0u) {

	UploadBuffer buffer = UploadBuffer(logicalDevice);
	buffer.CreateBuffer(logicalDevice, 1u);

	VkMemoryRequirements memoryReq = {};
	vkGetBufferMemoryRequirements(logicalDevice, buffer.GetBuffer(), &memoryReq);

	m_pBufferMemory = std::make_unique<DeviceMemory>(
		logicalDevice, physicalDevice, memoryReq, true
		);
}

void _CpuBaseBuffers::CreateBuffers(VkDevice device) {
	m_pBufferMemory->AllocateMemory(m_currentOffset);

	VkDeviceMemory uploadMemory = m_pBufferMemory->GetMemoryHandle();

	vkMapMemory(
		device, uploadMemory,
		0u, VK_WHOLE_SIZE, 0u,
		reinterpret_cast<void**>(&m_cpuHandle)
	);

	VkResult result;
	for (size_t index = 0u; index < std::size(m_pBuffers); ++index)
		VK_THROW_FAILED(result,
			vkBindBufferMemory(
				device, m_pBuffers[index]->GetBuffer(),
				uploadMemory, m_allocationData[index].offset
			)
		);

	AfterCreationStuff();
}

void _CpuBaseBuffers::FlushMemory(VkDevice device) {
	VkDeviceMemory uploadMemory = m_pBufferMemory->GetMemoryHandle();

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = uploadMemory;
	memoryRange.offset = 0u;
	memoryRange.size = VK_WHOLE_SIZE;

	VkResult result;
	VK_THROW_FAILED(result, vkFlushMappedMemoryRanges(device, 1u, &memoryRange));
}

void _CpuBaseBuffers::AfterCreationStuff() {}

// Upload Buffers

UploadBuffers::UploadBuffers(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice
) : _CpuBaseBuffers(logicalDevice, physicalDevice) {}

void UploadBuffers::CopyData() noexcept {
	for (size_t index = 0u; index < std::size(m_allocationData); ++index)
		memcpy(
			m_cpuHandle + m_allocationData[index].offset, m_dataHandles[index].get(),
			m_allocationData[index].bufferSize
		);
}

void UploadBuffers::AddBuffer(
	VkDevice device, std::unique_ptr<std::uint8_t> dataHandles, size_t bufferSize
) {
	std::shared_ptr<UploadBuffer> uploadBuffer = std::make_shared<UploadBuffer>(device);

	uploadBuffer->CreateBuffer(device, bufferSize);

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device, uploadBuffer->GetBuffer(), &memoryRequirements);

	m_currentOffset = Align(m_currentOffset, memoryRequirements.alignment);

	m_allocationData.emplace_back(bufferSize, m_currentOffset);

	m_currentOffset += memoryRequirements.size;

	m_pBuffers.emplace_back(std::move(uploadBuffer));

	m_dataHandles.emplace_back(std::move(dataHandles));
}

const std::vector<std::shared_ptr<UploadBuffer>>& UploadBuffers::GetUploadBuffers(
) const noexcept {
	return m_pBuffers;
}

// Host accessible Buffers
HostAccessibleBuffers::HostAccessibleBuffers(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice
) : _CpuBaseBuffers(logicalDevice, physicalDevice) {}

std::shared_ptr<UploadBuffer> HostAccessibleBuffers::AddBuffer(
	VkDevice device, size_t bufferSize,
	VkBufferUsageFlags bufferStageFlag
) {
	std::shared_ptr<UploadBuffer> hostBuffer = std::make_shared<UploadBuffer>(device);

	bufferStageFlag |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	hostBuffer->CreateBuffer(device, bufferSize, bufferStageFlag);

	m_pBuffers.emplace_back(hostBuffer);

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device, hostBuffer->GetBuffer(), &memoryRequirements);

	m_currentOffset = Align(m_currentOffset, memoryRequirements.alignment);

	bufferSize = memoryRequirements.size;

	m_allocationData.emplace_back(bufferSize, m_currentOffset);

	m_currentOffset += bufferSize;

	return hostBuffer;
}

void HostAccessibleBuffers::AfterCreationStuff() {
	for (size_t index = 0u; index < std::size(m_pBuffers); ++index)
		m_pBuffers[index]->SetCpuHandle(m_cpuHandle + m_allocationData[index].offset);
}

void HostAccessibleBuffers::ResetBufferData() noexcept {
	m_pBuffers = std::vector<std::shared_ptr<UploadBuffer>>();

	m_allocationData = std::vector<BufferData>();
}
