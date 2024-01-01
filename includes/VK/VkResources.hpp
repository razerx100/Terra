#ifndef VK_RESOURCES_HPP_
#define VK_RESOURCES_HPP_
#include <vulkan/vulkan.hpp>
#include <queue>
#include <VkAllocator.hpp>

template<typename ResourceType>
class VkBaseResource {
public:
	[[nodiscard]]
	ResourceType GetResource() const noexcept {
		return m_resource;
	}

protected:
	VkBaseResource() noexcept : m_resource{ VK_NULL_HANDLE } {}
	VkBaseResource(ResourceType resource) noexcept : m_resource{ resource } {}

protected:
	ResourceType m_resource;
};

class VkResource : public VkBaseResource<VkBuffer> {
public:
	VkResource(VkDevice device) noexcept;
	~VkResource() noexcept;

	VkResource(const VkResource&) = delete;
	VkResource& operator=(const VkResource&) = delete;

	VkResource(VkResource&& resource) noexcept;
	VkResource& operator=(VkResource&& resource) noexcept;

	void CreateResource(
		VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void CleanUpResource() noexcept;

	[[nodiscard]]
	VkMemoryRequirements GetMemoryRequirements(VkDevice device) const noexcept;

private:
	VkDevice m_deviceRef;
};

class VkImageResource : public VkBaseResource<VkImage> {
public:
	VkImageResource(VkDevice device) noexcept;
	~VkImageResource() noexcept;

	VkImageResource(const VkImageResource&) = delete;
	VkImageResource& operator=(const VkImageResource&) = delete;

	VkImageResource(VkImageResource&& resource) noexcept;
	VkImageResource& operator=(VkImageResource&& resource) noexcept;

	void CreateResource(
		VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void CleanUpResource() noexcept;

	[[nodiscard]]
	VkMemoryRequirements GetMemoryRequirements(VkDevice device) const noexcept;

private:
	VkDevice m_deviceRef;
};

// Newer ones
class Resource
{
public:
	Resource(MemoryManager& memoryManager, VkMemoryPropertyFlagBits memoryType);
	~Resource() noexcept;

	[[nodiscard]]
	inline VkDeviceSize Size() const noexcept { return m_allocationInfo.size; }
	[[nodiscard]]
	inline VkDeviceSize Offset() const noexcept { return m_allocationInfo.gpuOffset; }
	[[nodiscard]]
	inline std::uint8_t* CPUHandle() const noexcept { return m_allocationInfo.cpuOffset; }

protected:
	MemoryManager&                  m_memoryManager;
	MemoryManager::MemoryAllocation m_allocationInfo;
	VkMemoryPropertyFlagBits        m_resourceType;
};

class Buffer : public Resource
{
public:
	Buffer(VkDevice device, MemoryManager& memoryManager, VkMemoryPropertyFlagBits memoryType);
	~Buffer() noexcept;

	void Create(
		VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);

	[[nodiscard]]
	inline VkBuffer Get() const noexcept { return m_buffer; }

private:
	VkBuffer m_buffer;
	VkDevice m_device;
};

class Texture : public Resource
{
public:
	Texture(VkDevice device, MemoryManager& memoryManager, VkMemoryPropertyFlagBits memoryType);
	~Texture() noexcept;

	void Create(
		VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
	);

	[[nodiscard]]
	inline VkImage Get() const noexcept { return m_image; }

private:
	VkImage  m_image;
	VkDevice m_device;
};
#endif
