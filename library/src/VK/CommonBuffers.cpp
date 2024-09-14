#include <ranges>
#include <algorithm>

#include <CommonBuffers.hpp>

// Material Buffers
void MaterialBuffers::SetDescriptorBufferLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t bindingSlot, size_t setLayoutIndex
) const noexcept {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.AddBinding(
			bindingSlot, setLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
		);
}

void MaterialBuffers::SetDescriptorBuffer(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t bindingSlot, size_t setLayoutIndex
) const {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.SetStorageBufferDescriptor(m_buffers, bindingSlot, setLayoutIndex, 0u);
}

void MaterialBuffers::CreateBuffer(size_t materialCount)
{
	constexpr size_t strideSize    = GetStride();
	const auto materialBuffersSize = static_cast<VkDeviceSize>(strideSize * materialCount);

	Buffer newBuffer = GetCPUResource<Buffer>(m_device, m_memoryManager);
	newBuffer.Create(materialBuffersSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

	// All of the old materials will be only copied if the new buffer is larger.
	const VkDeviceSize oldBufferSize = m_buffers.BufferSize();

	if (oldBufferSize && newBuffer.BufferSize() > oldBufferSize)
		memcpy(newBuffer.CPUHandle(), m_buffers.CPUHandle(), m_buffers.BufferSize());

	m_buffers = std::move(newBuffer);
}

void MaterialBuffers::Update(size_t index) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle();
	constexpr size_t strideSize = GetStride();
	size_t materialOffset       = index * strideSize;

	if (m_elements.IsInUse(index))
	{
		const std::shared_ptr<Material>& material = m_elements.at(index);
		const MaterialData materialData           = material->Get();

		memcpy(bufferOffset + materialOffset, &materialData, strideSize);
	}
}

void MaterialBuffers::Update(const std::vector<size_t>& indices) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle();
	constexpr size_t strideSize = GetStride();

	for (size_t index : indices)
	{
		if (m_elements.IsInUse(index))
		{
			const std::shared_ptr<Material>& material = m_elements.at(index);
			const MaterialData materialData           = material->Get();

			const size_t materialOffset = index * strideSize;
			memcpy(bufferOffset + materialOffset, &materialData, strideSize);
		}
	}
}

// Shared Buffer Allocator
void SharedBufferAllocator::AddAllocInfo(VkDeviceSize offset, VkDeviceSize size) noexcept
{
	auto result = std::ranges::upper_bound(
		m_availableMemory, size, {},
		[](const AllocInfo& info) { return info.size; }
	);

	m_availableMemory.insert(result, AllocInfo{ offset, size });
}

std::optional<size_t> SharedBufferAllocator::GetAvailableAllocInfo(VkDeviceSize size) const noexcept
{
	auto result = std::ranges::lower_bound(
		m_availableMemory, size, {},
		[](const AllocInfo& info) { return info.size; }
	);

	if (result != std::end(m_availableMemory))
		return std::distance(std::begin(m_availableMemory), result);
	else
		return {};
}

SharedBufferAllocator::AllocInfo SharedBufferAllocator::GetAndRemoveAllocInfo(size_t index) noexcept
{
	AllocInfo allocInfo = m_availableMemory[index];

	m_availableMemory.erase(std::next(std::begin(m_availableMemory), index));

	return allocInfo;
}

VkDeviceSize SharedBufferAllocator::AllocateMemory(
	const AllocInfo& allocInfo, VkDeviceSize size
) noexcept {
	const VkDeviceSize offset     = allocInfo.offset;
	const VkDeviceSize freeMemory = allocInfo.size - size;

	if (freeMemory)
		AddAllocInfo(offset + size, freeMemory);

	return offset;
}

// Shared Buffer GPU
void SharedBufferGPU::CreateBuffer(VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer)
{
	// Moving it into the temp, as we will want to copy it back to the new bigger buffer.

	// This part is really important. So, the temp buffer should be null after a copy is done
	// and it has been destroyed. If it is at the beginning and the old buffer didn't have any
	// data, just moving wouldn't be an issue. But once it has some data, if we try to increase
	// the buffer size twice, it will move the old buffer with the data to the tempbuffer, but
	// if the GPU copy hasn't been done before it is increased again, the old tempBuffer will
	// be replaced with a new empty one and all data will be lost.
	// But this check should fix everything in theory. As, if we are recreating the main buffer
	// multiple times, the new old buffer would be empty as the gpu copy wouldn't have been done yet.
	// So, we wouldn't need to copy it. And since the first old buffer will be stored in the temp
	// buffer, it won't be null and so we wouldn't replace it and the data should be preserved and
	// safely copied to the main buffer upon calling CopyOldBuffer next.
	// Also no need to copy if the main buffer doesn't exist.
	if (m_buffer.Get() != VK_NULL_HANDLE && m_tempBuffer == nullptr)
	{
		m_tempBuffer = std::make_shared<Buffer>(std::move(m_buffer));
		tempBuffer.Add(m_tempBuffer);
	}

	m_buffer = GetGPUResource<Buffer>(m_device, m_memoryManager);
	m_buffer.Create(size, m_usageFlags, m_queueFamilyIndices);
}

VkDeviceSize SharedBufferGPU::ExtendBuffer(VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer)
{
	// I probably don't need to worry about aligning here, since it's all inside a single buffer?
	const VkDeviceSize oldSize = m_buffer.BufferSize();
	const VkDeviceSize offset  = oldSize;
	const VkDeviceSize newSize = oldSize + size;

	// If the alignment is 16bytes, at least 16bytes will be allocated. If the requested size
	// is bigger, then there shouldn't be any issues. But if the requested size is smaller,
	// the offset would be correct, but the buffer would be unnecessarily recreated, even though
	// it is not necessary. So, putting a check here.
	if (newSize > oldSize)
		CreateBuffer(newSize, tempBuffer);

	return offset;
}

void SharedBufferGPU::CopyOldBuffer(const VKCommandBuffer& copyBuffer) noexcept
{
	if (m_tempBuffer)
	{
		std::shared_ptr<Buffer> tempBuffer = std::move(m_tempBuffer);
		copyBuffer.CopyWhole(*tempBuffer, m_buffer);
	}
}

SharedBufferData SharedBufferGPU::AllocateAndGetSharedData(
	VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer
) {
	auto availableAllocIndex = m_allocator.GetAvailableAllocInfo(size);
	SharedBufferAllocator::AllocInfo allocInfo{ .offset = 0u, .size = 0u };

	if (!availableAllocIndex)
	{
		allocInfo.size   = size;
		allocInfo.offset = ExtendBuffer(size, tempBuffer);
	}
	else
		allocInfo = m_allocator.GetAndRemoveAllocInfo(*availableAllocIndex);

	return SharedBufferData{
		.bufferData = &m_buffer,
		.offset     = m_allocator.AllocateMemory(allocInfo, size),
		.size       = size
	};
}
