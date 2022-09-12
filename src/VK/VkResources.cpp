#include <VkResources.hpp>
#include <VKThrowMacros.hpp>

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

	VkResult result{};
	VK_THROW_FAILED(result,
		vkCreateBuffer(device, &createInfo, nullptr, &m_resource)
	);
}

void VkResource::CleanUpResource() noexcept {
	vkDestroyBuffer(m_deviceRef, m_resource, nullptr);
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

	VkResult result{};
	VK_THROW_FAILED(result,
		vkCreateImage(device, &createInfo, nullptr, &m_resource)
	);
}

void VkImageResource::CleanUpResource() noexcept {
	vkDestroyImage(m_deviceRef, m_resource, nullptr);
}
