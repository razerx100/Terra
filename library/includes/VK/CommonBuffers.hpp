#ifndef COMMON_BUFFERS_HPP_
#define COMMON_BUFFERS_HPP_
#include <VkResources.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <ReusableVkBuffer.hpp>
#include <VkCommandQueue.hpp>
#include <queue>

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

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot
	) const noexcept;

	// Shouldn't be called on every frame. Only updates the index specified.
	void Update(size_t index) const noexcept;
	// Shouldn't be called on every frame. Only updates the indices specified.
	void Update(const std::vector<size_t>& indices) const noexcept;

private:
	struct MaterialBufferData
	{
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 specular;
		UVInfo            diffuseTexUVInfo;
		UVInfo            specularTexUVInfo;
		std::uint32_t     diffuseTexIndex;
		std::uint32_t     specularTexIndex;
		float             shininess;
		float             padding;
	};

private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(MaterialBufferData); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t materialCount);
	void _remove(size_t index) noexcept;

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

class MeshBoundsBuffers
{
public:
	MeshBoundsBuffers(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_boundsBuffer{ GetCPUResource<Buffer>(device, memoryManager) },
		m_boundsData{}, m_availableIndices{}
	{}

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot
	) const noexcept;

	[[nodiscard]]
	// Returns the index of the Bounds in the BoundsBuffer.
	size_t Add(const MeshBounds& bounds) noexcept;
	[[nodiscard]]
	// Returns the indices of the bounds in the BoundsBuffer.
	std::vector<size_t> AddMultiple(const std::vector<MeshBounds>& multipleBounds) noexcept;

	void Remove(size_t index) noexcept;
	// Shouldn't be called on every frame. Update all the queued data.
	void Update() noexcept;

private:
	struct BoundsData
	{
		DirectX::XMFLOAT3 positiveBounds;
		float             padding0;
		DirectX::XMFLOAT3 negativeBounds;
		std::uint32_t     index; // This is actually a padding on the GPU, so can use it on the CPU.
		// GLSL's vec3 is actually vec4.
	};

private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(BoundsData); }
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	[[nodiscard]]
	size_t GetCount() const noexcept { return std::size(m_availableIndices); }

	void CreateBuffer(size_t boundsCount);

	[[nodiscard]]
	std::optional<size_t> GetFirstAvailableIndex() const noexcept
	{
		return ::GetFirstAvailableIndex(m_availableIndices);
	}
	[[nodiscard]]
	std::vector<size_t> GetAvailableIndices() const noexcept
	{
		return ::GetAvailableIndices(m_availableIndices);
	}

private:
	VkDevice               m_device;
	MemoryManager*         m_memoryManager;
	Buffer                 m_boundsBuffer;
	std::queue<BoundsData> m_boundsData;
	std::vector<bool>      m_availableIndices;

public:
	MeshBoundsBuffers(const MeshBoundsBuffers&) = delete;
	MeshBoundsBuffers& operator=(const MeshBoundsBuffers&) = delete;

	MeshBoundsBuffers(MeshBoundsBuffers&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_boundsBuffer{ std::move(other.m_boundsBuffer) },
		m_boundsData{ std::move(other.m_boundsData) },
		m_availableIndices{ std::move(other.m_availableIndices) }
	{}
	MeshBoundsBuffers& operator=(MeshBoundsBuffers&& other) noexcept
	{
		m_device           = other.m_device;
		m_memoryManager    = other.m_memoryManager;
		m_boundsBuffer     = std::move(other.m_boundsBuffer);
		m_boundsData       = std::move(other.m_boundsData);
		m_availableIndices = std::move(other.m_availableIndices);

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
		m_buffer{ GetGPUResource<Buffer>(device, memoryManager) },
		m_tempBuffer{ GetGPUResource<Buffer>(device, memoryManager) },
		m_usageFlags{ usageFlags }, m_queueFamilyIndices{ queueIndices }, m_availableMemory{}
	{}

	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	VkDeviceSize AllocateMemory(VkDeviceSize size);
	[[nodiscard]]
	// The offset from the start of the buffer will be returned.
	SharedBufferData AllocateAndGetSharedData(VkDeviceSize size)
	{
		return SharedBufferData{
			.bufferData = &m_buffer,
			.offset     = AllocateMemory(size),
			.size       = size
		};
	}

	void RelinquishMemory(VkDeviceSize offset, VkDeviceSize size) noexcept;
	void RelinquishMemory(const SharedBufferData& sharedData) noexcept
	{
		RelinquishMemory(sharedData.offset, sharedData.size);
	}

	void CopyOldBuffer(VKCommandBuffer& copyBuffer) const noexcept;
	void CleanupTempData() noexcept;

	[[nodiscard]]
	VkDeviceSize Size() const noexcept
	{
		return m_buffer.Size();
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
	void CreateBuffer(VkDeviceSize size);

	void AddAllocInfo(VkDeviceSize offset, VkDeviceSize size) noexcept;

private:
	VkDevice                   m_device;
	MemoryManager*             m_memoryManager;
	Buffer                     m_buffer;
	Buffer                     m_tempBuffer;
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
