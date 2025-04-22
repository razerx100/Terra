#ifndef VK_SHARED_BUFFERS_HPP_
#define VK_SHARED_BUFFERS_HPP_
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VkReusableBuffer.hpp>
#include <VkCommandQueue.hpp>
#include <queue>
#include <TemporaryDataBuffer.hpp>
#include <SharedBufferAllocator.hpp>

#include <MeshBundle.hpp>

namespace Terra
{
struct SharedBufferData
{
	Buffer const* bufferData;
	VkDeviceSize  offset;
	VkDeviceSize  size;
};

class SharedBufferBase
{
protected:
	SharedBufferBase(
		VkDevice device, MemoryManager* memoryManager, VkBufferUsageFlags usageFlags,
		const std::vector<std::uint32_t>& queueIndices, Buffer&& buffer
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ std::move(buffer) },
		// When a new bigger buffer is created to fit new data, a copy will be required. The old
		// one will be copied to the new bigger one. So, they will require both TRANSFER_DST and SRC
		// bits. Cus the same buffer would be the dst when it is created anew and then later be the
		// src when a bigger one is created.
		m_usageFlags{
			usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		},
		m_queueFamilyIndices{ queueIndices }, m_allocator{}
	{}

public:
	[[nodiscard]]
	VkDeviceSize Size() const noexcept
	{
		return m_buffer.BufferSize();
	}
	[[nodiscard]]
	VkDeviceAddress GetPhysicalAddress() const noexcept
	{
		return m_buffer.GpuPhysicalAddress();
	}

	[[nodiscard]]
	const Buffer& GetBuffer() const noexcept
	{
		return m_buffer;
	}

	[[nodiscard]]
	VkBuffer GetVkbuffer() const noexcept
	{
		return m_buffer.Get();
	}

protected:
	VkDevice                        m_device;
	MemoryManager*                  m_memoryManager;
	Buffer                          m_buffer;
	VkBufferUsageFlags              m_usageFlags;
	std::vector<std::uint32_t>      m_queueFamilyIndices;
	Callisto::SharedBufferAllocator m_allocator;

public:
	SharedBufferBase(const SharedBufferBase&) = delete;
	SharedBufferBase& operator=(const SharedBufferBase&) = delete;

	SharedBufferBase(SharedBufferBase&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) }, m_usageFlags{ other.m_usageFlags },
		m_queueFamilyIndices{ std::move(other.m_queueFamilyIndices) },
		m_allocator{ std::move(other.m_allocator) }
	{}
	SharedBufferBase& operator=(SharedBufferBase&& other) noexcept
	{
		m_device             = other.m_device;
		m_memoryManager      = other.m_memoryManager;
		m_buffer             = std::move(other.m_buffer);
		m_usageFlags         = other.m_usageFlags;
		m_queueFamilyIndices = std::move(other.m_queueFamilyIndices);
		m_allocator          = std::move(other.m_allocator);

		return *this;
	}
};

class SharedBufferGPU : public SharedBufferBase
{
public:
	SharedBufferGPU(
		VkDevice device, MemoryManager* memoryManager, VkBufferUsageFlags usageFlags,
		const std::vector<std::uint32_t>& queueIndices
	) : SharedBufferBase{
			device, memoryManager, usageFlags, queueIndices,
			GetGPUResource<Buffer>(device, memoryManager)
		}, m_oldBuffer{}
	{}

	void CopyOldBuffer(const VKCommandBuffer& copyBuffer) noexcept;

	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	SharedBufferData AllocateAndGetSharedData(
		VkDeviceSize size, Callisto::TemporaryDataBufferGPU& tempBuffer
	);

	void RelinquishMemory(const SharedBufferData& sharedData) noexcept
	{
		m_allocator.RelinquishMemory(sharedData.offset, sharedData.size);
	}

private:
	void CreateBuffer(VkDeviceSize size, Callisto::TemporaryDataBufferGPU& tempBuffer);
	[[nodiscard]]
	VkDeviceSize ExtendBuffer(VkDeviceSize size, Callisto::TemporaryDataBufferGPU& tempBuffer);

private:
	std::shared_ptr<Buffer> m_oldBuffer;

public:
	SharedBufferGPU(const SharedBufferGPU&) = delete;
	SharedBufferGPU& operator=(const SharedBufferGPU&) = delete;

	SharedBufferGPU(SharedBufferGPU&& other) noexcept
		: SharedBufferBase{ std::move(other) },
		m_oldBuffer{ std::move(other.m_oldBuffer) }
	{}
	SharedBufferGPU& operator=(SharedBufferGPU&& other) noexcept
	{
		SharedBufferBase::operator=(std::move(other));
		m_oldBuffer = std::move(other.m_oldBuffer);

		return *this;
	}
};

template<VkMemoryPropertyFlagBits MemoryType>
class SharedBufferWriteOnly : public SharedBufferBase
{
public:
	SharedBufferWriteOnly(
		VkDevice device, MemoryManager* memoryManager, VkBufferUsageFlags usageFlags,
		const std::vector<std::uint32_t>& queueIndices
	) : SharedBufferBase{
			device, memoryManager, usageFlags, queueIndices,
			Buffer{ device, memoryManager, MemoryType }
		}
	{}

	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	SharedBufferData AllocateAndGetSharedData(VkDeviceSize size, bool copyOldBuffer = false)
	{
		auto availableAllocIndex = m_allocator.GetAvailableAllocInfo(size);

		Callisto::SharedBufferAllocator::AllocInfo allocInfo{ .offset = 0u, .size = 0u };

		if (!availableAllocIndex)
		{
			allocInfo.size   = size;
			allocInfo.offset = ExtendBuffer(size, copyOldBuffer);
		}
		else
			allocInfo = m_allocator.GetAndRemoveAllocInfo(*availableAllocIndex);

		return SharedBufferData{
			.bufferData = &m_buffer,
			.offset     = m_allocator.AllocateMemory(allocInfo, size),
			.size       = size
		};
	}

	void RelinquishMemory(const SharedBufferData& sharedData) noexcept
	{
		m_allocator.RelinquishMemory(sharedData.offset, sharedData.size);
	}

private:
	[[nodiscard]]
	VkDeviceSize ExtendBuffer(VkDeviceSize size, bool copyOldBuffer)
	{
		// I probably don't need to worry about aligning here, since it's all inside a single
		// buffer?
		const VkDeviceSize oldSize = m_buffer.BufferSize();
		const VkDeviceSize offset  = oldSize;
		const VkDeviceSize newSize = oldSize + size;

		// If the alignment is 16bytes, at least 16bytes will be allocated. If the requested size
		// is bigger, then there shouldn't be any issues. But if the requested size is smaller,
		// the offset would be correct, but the buffer would be unnecessarily recreated, even though
		// it is not necessary. So, putting a check here.
		if (newSize > oldSize)
		{
			Buffer newBuffer{ m_device, m_memoryManager, MemoryType };

			newBuffer.Create(newSize, m_usageFlags, m_queueFamilyIndices);

			if (copyOldBuffer)
				memcpy(newBuffer.CPUHandle(), m_buffer.CPUHandle(), static_cast<size_t>(oldSize));

			m_buffer = std::move(newBuffer);
		}

		return offset;
	}

public:
	SharedBufferWriteOnly(const SharedBufferWriteOnly&) = delete;
	SharedBufferWriteOnly& operator=(const SharedBufferWriteOnly&) = delete;

	SharedBufferWriteOnly(SharedBufferWriteOnly&& other) noexcept
		: SharedBufferBase{ std::move(other) }
	{}
	SharedBufferWriteOnly& operator=(SharedBufferWriteOnly&& other) noexcept
	{
		SharedBufferBase::operator=(std::move(other));

		return *this;
	}
};

typedef SharedBufferWriteOnly<VK_MEMORY_PROPERTY_HOST_COHERENT_BIT> SharedBufferCPU;
typedef SharedBufferWriteOnly<VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT>  SharedBufferGPUWriteOnly;
}
#endif
