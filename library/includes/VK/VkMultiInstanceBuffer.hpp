#ifndef VK_MULTI_INSTANCE_BUFFER_HPP_
#define VK_MULTI_INSTANCE_BUFFER_HPP_
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <utility>
#include <concepts>

namespace Terra
{
// Currently can only be used on a single type of command queue.
class MultiInstanceCPUBuffer
{
public:
	MultiInstanceCPUBuffer(
		VkDevice device, MemoryManager* memoryManager, std::uint32_t instanceCount,
		std::uint32_t strideSize
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_instanceSize{ 0u }, m_instanceCount{ instanceCount }, m_strideSize{ strideSize }
	{}

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot, size_t setLayoutIndex
	) const {
		descriptorBuffer.SetStorageBufferDescriptor(m_buffer, bindingSlot, setLayoutIndex, 0u);
	}

	std::uint8_t* GetInstancePtr(VkDeviceSize instanceIndex) const noexcept
	{
		return m_buffer.CPUHandle() + (m_instanceSize * instanceIndex);
	}

	void AllocateForIndex(size_t index)
	{
		const VkDeviceSize currentSize = m_buffer.BufferSize();
		const size_t strideSize        = GetStride();

		// The index of the first element will be 0, so need to add the space for an
		// extra element.
		const auto minimumSpaceRequirement = static_cast<VkDeviceSize>(
			index * strideSize + strideSize
		);

		if (currentSize < minimumSpaceRequirement)
			CreateBuffer(index + 1u + GetExtraElementAllocationCount());
	}

private:
	[[nodiscard]]
	std::uint32_t GetStride() const noexcept { return m_strideSize; }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t elementCount)
	{
		const size_t strideSize        = GetStride();
		m_instanceSize                 = static_cast<VkDeviceSize>(strideSize * elementCount);
		const VkDeviceSize buffersSize = m_instanceSize * m_instanceCount;

		m_buffer.Create(buffersSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
	}

private:
	VkDevice       m_device;
	MemoryManager* m_memoryManager;
	Buffer         m_buffer;
	VkDeviceSize   m_instanceSize;
	std::uint32_t  m_instanceCount;
	// Hopefully the stride won't be bigger than 4GB?
	std::uint32_t  m_strideSize;

public:
	MultiInstanceCPUBuffer(const MultiInstanceCPUBuffer&) = delete;
	MultiInstanceCPUBuffer& operator=(const MultiInstanceCPUBuffer&) = delete;

	MultiInstanceCPUBuffer(MultiInstanceCPUBuffer&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) },
		m_instanceSize{ other.m_instanceSize },
		m_instanceCount{ other.m_instanceCount },
		m_strideSize{ other.m_strideSize }
	{}
	MultiInstanceCPUBuffer& operator=(MultiInstanceCPUBuffer&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;
		m_buffer        = std::move(other.m_buffer);
		m_instanceSize  = other.m_instanceSize;
		m_instanceCount = other.m_instanceCount;
		m_strideSize    = other.m_strideSize;

		return *this;
	}
};
}
#endif
