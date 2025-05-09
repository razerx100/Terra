#ifndef VK_REUSABLE_BUFFER_HPP_
#define VK_REUSABLE_BUFFER_HPP_
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <utility>
#include <concepts>

namespace Terra
{
template<typename T>
class MultiInstanceCPUBuffer
{
private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(T); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t elementCount)
	{
		constexpr size_t strideSize    = GetStride();
		m_instanceSize                 = static_cast<VkDeviceSize>(strideSize * elementCount);
		const VkDeviceSize buffersSize = m_instanceSize * m_instanceCount;

		m_buffer.Create(buffersSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
	}

public:
	MultiInstanceCPUBuffer(
		VkDevice device, MemoryManager* memoryManager, std::uint32_t instanceCount
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_instanceSize{ 0u }, m_instanceCount{ instanceCount }
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

	void ExtendBufferIfNecessaryFor(size_t index)
	{
		const VkDeviceSize currentSize = m_buffer.BufferSize();
		constexpr size_t strideSize    = GetStride();

		const auto minimumSpaceRequirement = static_cast<VkDeviceSize>(
			index * strideSize + strideSize
		);

		if (currentSize < minimumSpaceRequirement)
			CreateBuffer(index + 1u + GetExtraElementAllocationCount());
	}

private:
	VkDevice       m_device;
	MemoryManager* m_memoryManager;
	Buffer         m_buffer;
	VkDeviceSize   m_instanceSize;
	std::uint32_t  m_instanceCount;

public:
	MultiInstanceCPUBuffer(const MultiInstanceCPUBuffer&) = delete;
	MultiInstanceCPUBuffer& operator=(const MultiInstanceCPUBuffer&) = delete;

	MultiInstanceCPUBuffer(MultiInstanceCPUBuffer&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) },
		m_instanceSize{ other.m_instanceSize }, m_instanceCount{ other.m_instanceCount }
	{}
	MultiInstanceCPUBuffer& operator=(MultiInstanceCPUBuffer&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;
		m_buffer        = std::move(other.m_buffer);
		m_instanceSize  = other.m_instanceSize;
		m_instanceCount = other.m_instanceCount;

		return *this;
	}
};
}
#endif
