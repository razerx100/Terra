#include <VkResources.hpp>
#include <Exception.hpp>

// Static functions
template<typename CreateInfo>
static void ConfigureBufferQueueAccess(
	const std::vector<std::uint32_t>& queueFamilyIndices, CreateInfo& bufferInfo
) noexcept {
	const auto queueIndicesSize = static_cast<std::uint32_t>(std::size(queueFamilyIndices));

	if (queueIndicesSize > 1u)
	{
		bufferInfo.sharingMode           = VK_SHARING_MODE_CONCURRENT;
		bufferInfo.queueFamilyIndexCount = queueIndicesSize;
		bufferInfo.pQueueFamilyIndices   = std::data(queueFamilyIndices);
	}
	else
		bufferInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
}

// Vk Resource
VkResource::VkResource(VkDevice device) noexcept : m_deviceRef{ device } {}

VkResource::~VkResource() noexcept {
	CleanUpResource();
}

VkResource::VkResource(VkResource&& resource) noexcept
	: VkBaseResource<VkBuffer>{ resource.m_resource }, m_deviceRef { resource.m_deviceRef } {
	resource.m_resource = VK_NULL_HANDLE;
}

VkResource& VkResource::operator=(VkResource&& resource) noexcept {
	m_resource = resource.m_resource;
	m_deviceRef = resource.m_deviceRef;

	resource.m_resource = VK_NULL_HANDLE;

	return *this;
}

void VkResource::CreateResource(
	VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
	const std::vector<std::uint32_t>& queueFamilyIndices
) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = usageFlags;
	createInfo.size = bufferSize;

	ConfigureBufferQueueAccess(queueFamilyIndices, createInfo);

	vkCreateBuffer(device, &createInfo, nullptr, &m_resource);
}

void VkResource::CleanUpResource() noexcept {
	vkDestroyBuffer(m_deviceRef, m_resource, nullptr);
	m_resource = VK_NULL_HANDLE;
}

VkMemoryRequirements VkResource::GetMemoryRequirements(VkDevice device) const noexcept {
	VkMemoryRequirements memReq{};
	vkGetBufferMemoryRequirements(device, m_resource, &memReq);

	return memReq;
}

// Vk Image Resource
VkImageResource::VkImageResource(VkDevice device) noexcept : m_deviceRef{ device } {}

VkImageResource::~VkImageResource() noexcept {
	CleanUpResource();
}

VkImageResource::VkImageResource(VkImageResource&& resource) noexcept
	: VkBaseResource<VkImage>{ resource.m_resource }, m_deviceRef{ resource.m_deviceRef } {
	resource.m_resource = VK_NULL_HANDLE;
}

VkImageResource& VkImageResource::operator=(VkImageResource&& resource) noexcept {
	m_resource = resource.m_resource;
	m_deviceRef = resource.m_deviceRef;

	resource.m_resource = VK_NULL_HANDLE;

	return *this;
}

void VkImageResource::CreateResource(
	VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
	VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
) {
	VkImageCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.depth = 1u;
	createInfo.extent.height = height;
	createInfo.extent.width = width;
	createInfo.mipLevels = 1u;
	createInfo.arrayLayers = 1u;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usageFlags;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.flags = 0u;
	createInfo.format = imageFormat;

	ConfigureBufferQueueAccess(queueFamilyIndices, createInfo);

	vkCreateImage(device, &createInfo, nullptr, &m_resource);
}

void VkImageResource::CleanUpResource() noexcept {
	vkDestroyImage(m_deviceRef, m_resource, nullptr);
	m_resource = VK_NULL_HANDLE;
}

VkMemoryRequirements VkImageResource::GetMemoryRequirements(VkDevice device) const noexcept {
	VkMemoryRequirements memReq{};
	vkGetImageMemoryRequirements(device, m_resource, &memReq);

	return memReq;
}

// Resource
Resource::Resource(MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType)
	: m_memoryManager{ memoryManager }, m_allocationInfo{}, m_resourceType{ memoryType } {}

Resource::~Resource() noexcept
{
	if (m_memoryManager)
		m_memoryManager->Deallocate(m_allocationInfo, m_resourceType);
}

// Buffer
Buffer::Buffer(VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType)
	: Resource{ memoryManager, memoryType }, m_buffer{ VK_NULL_HANDLE }, m_device{ device } {}

Buffer::~Buffer() noexcept
{
	vkDestroyBuffer(m_device, m_buffer, nullptr);
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

	ConfigureBufferQueueAccess(queueFamilyIndices, createInfo);

	vkCreateBuffer(m_device, &createInfo, nullptr, &m_buffer);

	if (m_memoryManager)
		m_allocationInfo = m_memoryManager->AllocateBuffer(m_buffer, m_resourceType);
	else
		throw Exception("MemoryManager Exception", "Memory manager unavailable.");
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
	m_format{ VK_FORMAT_UNDEFINED }
{}

Texture::~Texture() noexcept
{
	vkDestroyImage(m_device, m_image, nullptr);
}

void Texture::Create(
	std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
	VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
) {
	VkExtent3D extent
	{
		.width  = width,
		.height = height,
		.depth  = 1u
	};

	VkImageCreateInfo createInfo
	{
		.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags         = 0u,
		.imageType     = VK_IMAGE_TYPE_2D,
		.format        = imageFormat,
		.extent        = extent,
		.mipLevels     = 1u,
		.arrayLayers   = 1u,
		.samples       = VK_SAMPLE_COUNT_1_BIT,
		.tiling        = VK_IMAGE_TILING_OPTIMAL,
		.usage         = usageFlags,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	m_format = imageFormat;

	ConfigureBufferQueueAccess(queueFamilyIndices, createInfo);

	vkCreateImage(m_device, &createInfo, nullptr, &m_image);

	if (m_memoryManager)
		m_allocationInfo = m_memoryManager->AllocateImage(m_image, m_resourceType);
	else
		throw Exception("MemoryManager Exception", "Memory manager unavailable.");
}
