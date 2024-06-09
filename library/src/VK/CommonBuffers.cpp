#include <ranges>
#include <algorithm>

#include <CommonBuffers.hpp>

// Material Buffers
void MaterialBuffers::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot
) const {
	descriptorBuffer.SetStorageBufferDescriptor(m_buffers, bindingSlot, 0u);
}

void MaterialBuffers::CreateBuffer(size_t materialCount)
{
	constexpr size_t strideSize    = GetStride();
	const auto materialBuffersSize = static_cast<VkDeviceSize>(strideSize * materialCount);

	Buffer newBuffer = GetCPUResource<Buffer>(m_device, m_memoryManager);
	newBuffer.Create(materialBuffersSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

	// All of the old materials will be only copied if the new buffer is larger.
	const VkDeviceSize oldBufferSize = m_buffers.Size();
	if (oldBufferSize && newBuffer.Size() > oldBufferSize)
	{
		memcpy(newBuffer.CPUHandle(), m_buffers.CPUHandle(), m_buffers.Size());
	}

	m_buffers = std::move(newBuffer);
}

void MaterialBuffers::_remove(size_t index) noexcept
{
	m_elements.RemoveElement(index, &std::shared_ptr<Material>::reset);
}

void MaterialBuffers::Update(size_t index) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle();
	constexpr size_t strideSize = GetStride();
	size_t materialOffset       = index * strideSize;

	auto& materials = m_elements.Get();

	const std::shared_ptr<Material>& material = materials.at(index);
	const MaterialData materialData           = material->Get();

	const MaterialBufferData bufferData{
		.ambient           = materialData.ambient,
		.diffuse           = materialData.diffuse,
		.specular          = materialData.specular,
		.diffuseTexUVInfo  = material->GetDiffuseUVInfo(),
		.specularTexUVInfo = material->GetSpecularUVInfo(),
		.diffuseTexIndex   = material->GetDiffuseIndex(),
		.specularTexIndex  = material->GetSpecularIndex(),
		.shininess         = materialData.shininess
	};

	memcpy(bufferOffset + materialOffset, &bufferData, strideSize);
}

void MaterialBuffers::Update(const std::vector<size_t>& indices) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle();
	constexpr size_t strideSize = GetStride();

	auto& materials = m_elements.Get();

	for (size_t index : indices)
	{
		const std::shared_ptr<Material>& material = materials.at(index);
		const MaterialData materialData           = material->Get();

		const MaterialBufferData bufferData{
			.ambient           = materialData.ambient,
			.diffuse           = materialData.diffuse,
			.specular          = materialData.specular,
			.diffuseTexUVInfo  = material->GetDiffuseUVInfo(),
			.specularTexUVInfo = material->GetSpecularUVInfo(),
			.diffuseTexIndex   = material->GetDiffuseIndex(),
			.specularTexIndex  = material->GetSpecularIndex(),
			.shininess         = materialData.shininess
		};

		const size_t materialOffset = index * strideSize;
		memcpy(bufferOffset + materialOffset, &bufferData, strideSize);
	}
}

// Shared Buffer
void SharedBuffer::CreateBuffer(VkDeviceSize size)
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
	if (m_tempBuffer.Get() == VK_NULL_HANDLE)
		m_tempBuffer = std::move(m_buffer);

	m_buffer = GetGPUResource<Buffer>(m_device, m_memoryManager);
	m_buffer.Create(size, m_usageFlags, m_queueFamilyIndices);
}

void SharedBuffer::CopyOldBuffer(VKCommandBuffer& copyBuffer) const noexcept
{
	copyBuffer.CopyWhole(m_tempBuffer, m_buffer);
}

void SharedBuffer::CleanupTempData() noexcept
{
	m_tempBuffer.Destroy();
}

VkDeviceSize SharedBuffer::AllocateMemory(VkDeviceSize size)
{
	auto result = std::ranges::lower_bound(
		m_availableMemory, size, {},
		[](const AllocInfo& info) { return info.size; }
	);

	VkDeviceSize offset = 0u;

	// I probably don't need to worry about aligning here, since it's all inside a single buffer?
	if (result == std::end(m_availableMemory))
	{
		offset = m_occupyingSize;

		const VkDeviceSize actualSize   = m_buffer.Size();
		const VkDeviceSize requiredSize = m_occupyingSize + size;

		m_occupyingSize = requiredSize;

		// If the alignment is 16bytes, at least 16bytes will be allocated. If the requested size
		// is bigger, then there shouldn't be any issues. But if the requested size is smaller,
		// the offset would be correct, but the buffer would be unnecessarily recreated, even though
		// it is not necessary. So, putting a check here.
		if (requiredSize > actualSize)
			CreateBuffer(requiredSize);
	}
	else
	{
		const AllocInfo& allocInfo = *result;
		offset = allocInfo.offset;

		AddAllocInfo(offset + size, allocInfo.size - size);
	}

	return offset;
}

void SharedBuffer::AddAllocInfo(VkDeviceSize offset, VkDeviceSize size) noexcept
{
	auto result = std::ranges::upper_bound(
		m_availableMemory, size, {},
		[](const AllocInfo& info) { return info.size; }
	);

	m_availableMemory.insert(result, AllocInfo{ offset, size });
}

void SharedBuffer::RelinquishMemory(VkDeviceSize offset, VkDeviceSize size) noexcept
{
	AddAllocInfo(offset, size);
}
