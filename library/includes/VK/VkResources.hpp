#ifndef VK_RESOURCES_HPP_
#define VK_RESOURCES_HPP_
#include <vulkan/vulkan.hpp>
#include <queue>
#include <type_traits>
#include <VkAllocator.hpp>

class Resource
{
public:
	Resource(MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType);
	~Resource() noexcept;

	[[nodiscard]]
	VkDeviceSize Size() const noexcept { return m_allocationInfo.size; }
	[[nodiscard]]
	VkDeviceSize GpuRelativeOffset() const noexcept { return m_allocationInfo.gpuOffset; }
	[[nodiscard]]
	std::uint8_t* CPUHandle() const noexcept { return m_allocationInfo.cpuOffset; }

protected:
	void Deallocate() noexcept;

	template<typename ResourceType>
	void Allocate(ResourceType resource)
	{
		if (m_memoryManager)
		{
			if constexpr (std::is_same_v<ResourceType, VkImage>)
				m_allocationInfo = m_memoryManager->AllocateImage(resource, m_resourceType);
			else if constexpr (std::is_same_v<ResourceType, VkBuffer>)
				m_allocationInfo = m_memoryManager->AllocateBuffer(resource, m_resourceType);

			m_hasAllocation = true;
		}
		else
			ThrowMemoryManagerException();
	}

	template<typename CreateInfo>
	static void ConfigureResourceQueueAccess(
		const std::vector<std::uint32_t>& queueFamilyIndices, CreateInfo& resourceInfo
	) noexcept {
		const auto queueIndicesSize = static_cast<std::uint32_t>(std::size(queueFamilyIndices));

		if (queueIndicesSize > 1u)
		{
			resourceInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
			resourceInfo.queueFamilyIndexCount = queueIndicesSize;
			resourceInfo.pQueueFamilyIndices   = std::data(queueFamilyIndices);
		}
		else
			resourceInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

private:
	void SelfDestruct() noexcept;
	void ThrowMemoryManagerException();

private:
	MemoryManager*                  m_memoryManager;
	MemoryManager::MemoryAllocation m_allocationInfo;
	VkMemoryPropertyFlagBits        m_resourceType;
	bool                            m_hasAllocation;

public:
	Resource(const Resource&) = delete;
	Resource& operator=(const Resource&) = delete;

	Resource(Resource&& other) noexcept
		: m_memoryManager{ std::exchange(other.m_memoryManager, nullptr) },
		m_allocationInfo{ other.m_allocationInfo },
		m_resourceType{ other.m_resourceType },
		m_hasAllocation{ std::exchange(other.m_hasAllocation, false) }
	{
		// Setting the allocation check to null, so the other object doesn't deallocate.
		// Don't need to deallocate our previous data, as there was none.
	}
	Resource& operator=(Resource&& other) noexcept
	{
		// Deallocating the already existing memory.
		SelfDestruct();

		m_memoryManager  = std::exchange(other.m_memoryManager, nullptr);
		m_allocationInfo = other.m_allocationInfo;
		m_resourceType   = other.m_resourceType;
		// Taking the allocation data from the other object.
		m_hasAllocation  = std::exchange(other.m_hasAllocation, false);
		// Setting the allocation check to null, so the other object doesn't deallocate.

		return *this;
	}
};

class Buffer : public Resource
{
public:
	Buffer(VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType);
	~Buffer() noexcept;

	void Create(
		VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);

	void Destroy() noexcept;

	[[nodiscard]]
	VkBuffer Get() const noexcept { return m_buffer; }
	[[nodiscard]]
	VkDeviceAddress GpuPhysicalAddress() const noexcept;

private:
	void SelfDestruct() noexcept;

private:
	VkBuffer m_buffer;
	VkDevice m_device;

public:
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;

	Buffer(Buffer&& other) noexcept
		: Resource{ std::move(other) }, m_buffer{ std::exchange(other.m_buffer, VK_NULL_HANDLE) },
		m_device{ other.m_device }
	{}
	Buffer& operator=(Buffer&& other) noexcept
	{
		Resource::operator=(std::move(other));

		SelfDestruct();

		m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
		m_device = other.m_device;

		return *this;
	}
};

class Texture : public Resource
{
public:
	Texture(VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType);
	~Texture() noexcept;

	void Create2D(
		std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels, VkFormat imageFormat,
		VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void Create3D(
		std::uint32_t width, std::uint32_t height, std::uint32_t depth,
		std::uint32_t mipLevels, VkFormat imageFormat,
		VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
	);

	void Destroy() noexcept;

	[[nodiscard]]
	VkImage Get() const noexcept { return m_image; }
	[[nodiscard]]
	VkFormat Format() const noexcept { return m_format; }
	[[nodiscard]]
	VkExtent3D GetExtent() const noexcept { return m_imageExtent; }

private:
	void SelfDestruct() noexcept;

	void Create(
		std::uint32_t width, std::uint32_t height, std::uint32_t depth,
		std::uint32_t mipLevels, VkFormat imageFormat, VkImageType imageType,
		VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
	);

private:
	VkImage    m_image;
	VkDevice   m_device;
	VkFormat   m_format;
	VkExtent3D m_imageExtent;

public:
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(Texture&& other) noexcept
		: Resource{ std::move(other) }, m_image{ std::exchange(other.m_image, VK_NULL_HANDLE) },
		m_device{ other.m_device }, m_format{ other.m_format }, m_imageExtent{ other.m_imageExtent }
	{}
	Texture& operator=(Texture&& other) noexcept
	{
		Resource::operator=(std::move(other));

		SelfDestruct();

		m_image       = std::exchange(other.m_image, VK_NULL_HANDLE);
		m_device      = other.m_device;
		m_format      = other.m_format;
		m_imageExtent = other.m_imageExtent;

		return *this;
	}
};

template<class T>
[[nodiscard]]
T GetCPUResource(VkDevice device, MemoryManager* memoryManager)
{
	return T{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
}

template<class T>
[[nodiscard]]
T GetGPUResource(VkDevice device, MemoryManager* memoryManager)
{
	return T{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
}
#endif
