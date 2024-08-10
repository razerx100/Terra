#ifndef COMMON_BUFFERS_HPP_
#define COMMON_BUFFERS_HPP_
#include <VkResources.hpp>
#include <StagingBufferManager.hpp>
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

class SharedBuffer
{
public:
	SharedBuffer(
		VkDevice device, MemoryManager* memoryManager, VkBufferUsageFlags usageFlags,
		const std::vector<std::uint32_t>& queueIndices
	) : m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ GetGPUResource<Buffer>(device, memoryManager) }, m_tempBuffer{},
		// When a new bigger buffer is created to fit new data, a copy will be required. The old
		// one will be copied to the new bigger one. So, they will require both TRANSFER_DST and SRC
		// bits. Cus the same buffer would be the dst when it is created anew and then later be the src
		// when a bigger one is created.
		m_usageFlags{ usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT },
		m_queueFamilyIndices{ queueIndices }, m_availableMemory{}
	{}

	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	VkDeviceSize AllocateMemory(VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer);
	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	SharedBufferData AllocateAndGetSharedData(VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer)
	{
		return SharedBufferData{
			.bufferData = &m_buffer,
			.offset     = AllocateMemory(size, tempBuffer),
			.size       = size
		};
	}

	void RelinquishMemory(VkDeviceSize offset, VkDeviceSize size) noexcept;
	void RelinquishMemory(const SharedBufferData& sharedData) noexcept
	{
		RelinquishMemory(sharedData.offset, sharedData.size);
	}

	void CopyOldBuffer(const VKCommandBuffer& copyBuffer) const noexcept;

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

private:
	struct AllocInfo
	{
		VkDeviceSize offset;
		VkDeviceSize size;
	};

private:
	void CreateBuffer(VkDeviceSize size, TemporaryDataBufferGPU& tempBuffer);

	void AddAllocInfo(VkDeviceSize offset, VkDeviceSize size) noexcept;

private:
	VkDevice                   m_device;
	MemoryManager*             m_memoryManager;
	Buffer                     m_buffer;
	std::shared_ptr<Buffer>    m_tempBuffer;
	VkBufferUsageFlags         m_usageFlags;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	std::vector<AllocInfo>     m_availableMemory;

public:
	SharedBuffer(const SharedBuffer&) = delete;
	SharedBuffer& operator=(const SharedBuffer&) = delete;

	SharedBuffer(SharedBuffer&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) },
		m_tempBuffer{ std::move(other.m_tempBuffer) },
		m_usageFlags{ other.m_usageFlags },
		m_queueFamilyIndices{ std::move(other.m_queueFamilyIndices) },
		m_availableMemory{ std::move(other.m_availableMemory) }
	{}
	SharedBuffer& operator=(SharedBuffer&& other) noexcept
	{
		m_device             = other.m_device;
		m_memoryManager      = other.m_memoryManager;
		m_buffer             = std::move(other.m_buffer);
		m_tempBuffer         = std::move(other.m_tempBuffer);
		m_usageFlags         = other.m_usageFlags;
		m_queueFamilyIndices = std::move(other.m_queueFamilyIndices);
		m_availableMemory    = std::move(other.m_availableMemory);

		return *this;
	}
};
#endif
