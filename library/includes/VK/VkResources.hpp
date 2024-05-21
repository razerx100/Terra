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
		: m_memoryManager{ other.m_memoryManager }, m_allocationInfo{ other.m_allocationInfo },
		m_resourceType{ other.m_resourceType }, m_hasAllocation{ other.m_hasAllocation }
	{
		// Setting the allocation check to null, so the other object doesn't deallocate.
		// Don't need to deallocate our previous data, as there was none.
		other.m_memoryManager = nullptr;
		other.m_hasAllocation = false;
	}
	Resource& operator=(Resource&& other) noexcept
	{
		// Deallocating the already existing memory.
		SelfDestruct();

		m_memoryManager       = other.m_memoryManager;
		m_allocationInfo      = other.m_allocationInfo;
		m_resourceType        = other.m_resourceType;
		// Taking the allocation data from the other object.
		m_hasAllocation       = other.m_hasAllocation;
		// Setting the allocation check to null, so the other object doesn't deallocate.
		other.m_memoryManager = nullptr;
		other.m_hasAllocation = false;

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
		: Resource{ std::move(other) }, m_buffer{ other.m_buffer }, m_device{ other.m_device }
	{
		other.m_buffer = VK_NULL_HANDLE;
	}
	Buffer& operator=(Buffer&& other) noexcept
	{
		Resource::operator=(std::move(other));

		SelfDestruct();

		m_buffer       = other.m_buffer;
		m_device       = other.m_device;
		other.m_buffer = VK_NULL_HANDLE;

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
		: Resource{ std::move(other) }, m_image{ other.m_image }, m_device{ other.m_device },
		m_format{ other.m_format }, m_imageExtent{ other.m_imageExtent }
	{
		other.m_image = VK_NULL_HANDLE;
	}
	Texture& operator=(Texture&& other) noexcept
	{
		Resource::operator=(std::move(other));

		SelfDestruct();

		m_image       = other.m_image;
		m_device      = other.m_device;
		m_format      = other.m_format;
		m_imageExtent = other.m_imageExtent;
		other.m_image = VK_NULL_HANDLE;

		return *this;
	}
};
#endif
