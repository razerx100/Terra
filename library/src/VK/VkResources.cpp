#include <VkResources.hpp>
#include <Exception.hpp>
#include <unordered_map>

// Resource
Resource::Resource(MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType)
	: m_memoryManager{ memoryManager },
	m_allocationInfo{
		.gpuOffset = 0u,
		.cpuOffset = nullptr,
		.size      = 0u,
		.alignment = 0u,
		.memoryID  = 0u,
		.isValid   = false
	},
	m_resourceType{ memoryType }
{}

Resource::~Resource() noexcept
{
	SelfDestruct();
}

void Resource::Deallocate() noexcept
{
	if (m_memoryManager && m_allocationInfo.isValid)
	{
		m_memoryManager->Deallocate(m_allocationInfo, m_resourceType);

		m_allocationInfo.isValid = false;
	}
}

void Resource::SelfDestruct() noexcept
{
	Deallocate();
}

void Resource::ThrowMemoryManagerException()
{
	throw Exception("MemoryManager Exception", "Memory manager unavailable.");
}

// Buffer
Buffer::Buffer(VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType)
	: Resource{ memoryManager, memoryType }, m_buffer{ VK_NULL_HANDLE }, m_device{ device },
	m_bufferSize{ 0u }
{}

Buffer::~Buffer() noexcept
{
	SelfDestruct();
}

void Buffer::SelfDestruct() noexcept
{
	vkDestroyBuffer(m_device, m_buffer, nullptr);

	m_buffer = VK_NULL_HANDLE;
}

void Buffer::Destroy() noexcept
{
	SelfDestruct();
	Deallocate();
}

void Buffer::Create(
	VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
	const std::vector<std::uint32_t>& queueFamilyIndices
) {
	VkBufferCreateInfo createInfo
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size  = bufferSize,
		.usage = usageFlags | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	};

	ConfigureResourceQueueAccess(queueFamilyIndices, createInfo);

	// If the buffer pointer is already allocated, then free it.
	if (m_buffer != VK_NULL_HANDLE)
		Destroy();

	vkCreateBuffer(m_device, &createInfo, nullptr, &m_buffer);

	m_bufferSize = bufferSize;

	Allocate(m_buffer);
}

VkDeviceAddress Buffer::GpuPhysicalAddress() const noexcept
{
	VkBufferDeviceAddressInfo testBufferInfo{
		.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = m_buffer
	};

	return vkGetBufferDeviceAddress(m_device, &testBufferInfo);
}

// Texture
Texture::Texture(VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType)
	: Resource{ memoryManager, memoryType }, m_image{ VK_NULL_HANDLE }, m_device{ device },
	m_format{ VK_FORMAT_UNDEFINED }, m_imageExtent{ 0u, 0u, 0u }
{}

Texture::~Texture() noexcept
{
	SelfDestruct();
}

void Texture::SelfDestruct() noexcept
{
	vkDestroyImage(m_device, m_image, nullptr);

	m_image = VK_NULL_HANDLE;
}

void Texture::Destroy() noexcept
{
	SelfDestruct();
	Deallocate();
}

void Texture::Create2D(
	std::uint32_t width, std::uint32_t height, std::uint32_t mipLevels, VkFormat imageFormat,
	VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
) {
	Create(
		width, height, 1u, mipLevels, imageFormat, VK_IMAGE_TYPE_2D, usageFlags, queueFamilyIndices
	);
}

void Texture::Create3D(
	std::uint32_t width, std::uint32_t height, std::uint32_t depth,
	std::uint32_t mipLevels, VkFormat imageFormat,
	VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
) {
	Create(
		width, height, depth, mipLevels, imageFormat, VK_IMAGE_TYPE_3D, usageFlags, queueFamilyIndices
	);
}

void Texture::Create(
	std::uint32_t width, std::uint32_t height, std::uint32_t depth,
	std::uint32_t mipLevels, VkFormat imageFormat, VkImageType imageType,
	VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
) {
	m_imageExtent.width  = width;
	m_imageExtent.height = height;
	m_imageExtent.depth  = depth;

	m_format = imageFormat;

	VkImageCreateInfo createInfo
	{
		.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags         = 0u,
		.imageType     = imageType,
		.format        = m_format,
		.extent        = m_imageExtent,
		.mipLevels     = mipLevels,
		.arrayLayers   = 1u,
		.samples       = VK_SAMPLE_COUNT_1_BIT,
		.tiling        = VK_IMAGE_TILING_OPTIMAL,
		.usage         = usageFlags,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	ConfigureResourceQueueAccess(queueFamilyIndices, createInfo);

	// If the image pointer is already allocated, then free it.
	if (m_image != VK_NULL_HANDLE)
		Destroy();

	vkCreateImage(m_device, &createInfo, nullptr, &m_image);

	Allocate(m_image);
}

VkDeviceSize Texture::GetBufferSize() const noexcept
{
	// For example: R8G8B8A8 has 4 components, 8bits at each component. So, 4bytes.
	const static std::unordered_map<VkFormat, std::uint32_t> formatSizeMap
	{
		{ VK_FORMAT_R8G8B8A8_UNORM, 4u },
		{ VK_FORMAT_R8G8B8A8_SNORM, 4u },
		{ VK_FORMAT_R8G8B8A8_UINT,  4u },
		{ VK_FORMAT_R8G8B8A8_SINT,  4u },
		{ VK_FORMAT_R8G8B8A8_SRGB,  4u },
		{ VK_FORMAT_B8G8R8A8_UNORM, 4u },
		{ VK_FORMAT_B8G8R8A8_SNORM, 4u },
		{ VK_FORMAT_B8G8R8A8_UINT,  4u },
		{ VK_FORMAT_B8G8R8A8_SINT,  4u },
		{ VK_FORMAT_B8G8R8A8_SRGB,  4u }
	};

	const VkFormat textureFormat = Format();

	auto formatSize = formatSizeMap.find(textureFormat);

	if (formatSize != std::end(formatSizeMap))
	{
		const VkDeviceSize sizePerPixel = formatSize->second;

		return static_cast<VkDeviceSize>(m_imageExtent.width)
			* m_imageExtent.height * m_imageExtent.depth * sizePerPixel;
	}

	// Not bothering with adding the size of every single colour format. If an format size isn't
	// defined here, then 0 will be returned.
	return 0u;
}
