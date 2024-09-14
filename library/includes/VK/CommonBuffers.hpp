#ifndef COMMON_BUFFERS_HPP_
#define COMMON_BUFFERS_HPP_
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <ReusableVkBuffer.hpp>
#include <VkCommandQueue.hpp>
#include <queue>
#include <TemporaryDataBuffer.hpp>

#include <Material.hpp>
#include <MeshBundle.hpp>

class MaterialBuffers : public ReusableVkBuffer<MaterialBuffers, std::shared_ptr<Material>>
{
	friend class ReusableVkBuffer<MaterialBuffers, std::shared_ptr<Material>>;
public:
	MaterialBuffers(VkDevice device, MemoryManager* memoryManager)
		: ReusableVkBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_device{ device }, m_memoryManager{ memoryManager }
	{}

	void SetDescriptorBufferLayout(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t bindingSlot,
		size_t setLayoutIndex
	) const noexcept;
	void SetDescriptorBuffer(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t bindingSlot,
		size_t setLayoutIndex
	) const;

	// Shouldn't be called on every frame. Only updates the index specified.
	void Update(size_t index) const noexcept;
	// Shouldn't be called on every frame. Only updates the indices specified.
	void Update(const std::vector<size_t>& indices) const noexcept;

private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(MaterialData); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t materialCount);

private:
	VkDevice       m_device;
	MemoryManager* m_memoryManager;

public:
	MaterialBuffers(const MaterialBuffers&) = delete;
	MaterialBuffers& operator=(const MaterialBuffers&) = delete;

	MaterialBuffers(MaterialBuffers&& other) noexcept
		: ReusableVkBuffer{ std::move(other) },
		m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager }
	{}
	MaterialBuffers& operator=(MaterialBuffers&& other) noexcept
	{
		ReusableVkBuffer::operator=(std::move(other));
		m_device                  = other.m_device;
		m_memoryManager           = other.m_memoryManager;

		return *this;
	}
};

struct SharedBufferData
{
	Buffer const* bufferData;
	VkDeviceSize  offset;
	VkDeviceSize  size;
};

class SharedBufferAllocator
{
public:
	struct AllocInfo
	{
		VkDeviceSize offset;
		VkDeviceSize size;
	};

public:
	SharedBufferAllocator() : m_availableMemory{} {}

	[[nodiscard]]
	// The offset from the start of the buffer will be returned. Should make sure
	// there is enough memory before calling this.
	VkDeviceSize AllocateMemory(const AllocInfo& allocInfo, VkDeviceSize size) noexcept;

	void AddAllocInfo(VkDeviceSize offset, VkDeviceSize size) noexcept;
	void RelinquishMemory(VkDeviceSize offset, VkDeviceSize size) noexcept
	{
		AddAllocInfo(offset, size);
	}

	[[nodiscard]]
	std::optional<size_t> GetAvailableAllocInfo(VkDeviceSize size) const noexcept;
	[[nodiscard]]
	AllocInfo GetAndRemoveAllocInfo(size_t index) noexcept;

private:
	std::vector<AllocInfo> m_availableMemory;

public:
	SharedBufferAllocator(const SharedBufferAllocator& other) noexcept
		: m_availableMemory{ other.m_availableMemory }
	{}
	SharedBufferAllocator& operator=(const SharedBufferAllocator& other) noexcept
	{
		m_availableMemory = other.m_availableMemory;

		return *this;
	}
	SharedBufferAllocator(SharedBufferAllocator&& other) noexcept
		: m_availableMemory{ std::move(other.m_availableMemory) }
	{}
	SharedBufferAllocator& operator=(SharedBufferAllocator&& other) noexcept
	{
		m_availableMemory = std::move(other.m_availableMemory);

		return *this;
	}
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
		// bits. Cus the same buffer would be the dst when it is created anew and then later be the src
		// when a bigger one is created.
		m_usageFlags{ usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
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
	VkDevice                   m_device;
	MemoryManager*             m_memoryManager;
	Buffer                     m_buffer;
	VkBufferUsageFlags         m_usageFlags;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	SharedBufferAllocator      m_allocator;

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
			device, memoryManager, usageFlags, queueIndices, GetGPUResource<Buffer>(device, memoryManager)
		}, m_tempBuffer{}
	{}

	void CopyOldBuffer(const VKCommandBuffer& copyBuffer) noexcept;

	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	SharedBufferData AllocateAndGetSharedData(VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer);

	void RelinquishMemory(const SharedBufferData& sharedData) noexcept
	{
		m_allocator.RelinquishMemory(sharedData.offset, sharedData.size);
	}

private:
	void CreateBuffer(VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer);
	[[nodiscard]]
	VkDeviceSize ExtendBuffer(VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer);

private:
	std::shared_ptr<Buffer> m_tempBuffer;

public:
	SharedBufferGPU(const SharedBufferGPU&) = delete;
	SharedBufferGPU& operator=(const SharedBufferGPU&) = delete;

	SharedBufferGPU(SharedBufferGPU&& other) noexcept
		: SharedBufferBase{ std::move(other) },
		m_tempBuffer{ std::move(other.m_tempBuffer) }
	{}
	SharedBufferGPU& operator=(SharedBufferGPU&& other) noexcept
	{
		SharedBufferBase::operator=(std::move(other));
		m_tempBuffer = std::move(other.m_tempBuffer);

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
			device, memoryManager, usageFlags, queueIndices, Buffer{ device, memoryManager, MemoryType }
		}
	{}

	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	SharedBufferData AllocateAndGetSharedData(VkDeviceSize size)
	{
		auto availableAllocIndex = m_allocator.GetAvailableAllocInfo(size);
		SharedBufferAllocator::AllocInfo allocInfo{ .offset = 0u, .size = 0u };

		if (!availableAllocIndex)
		{
			allocInfo.size   = size;
			allocInfo.offset = ExtendBuffer(size);
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
	void CreateBuffer(VkDeviceSize size)
	{
		m_buffer.Create(size, m_usageFlags, m_queueFamilyIndices);
	}

	[[nodiscard]]
	VkDeviceSize ExtendBuffer(VkDeviceSize size)
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
			CreateBuffer(newSize);

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
#endif
