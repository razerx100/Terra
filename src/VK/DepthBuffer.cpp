#include <DepthBuffer.hpp>
#include <VkHelperFunctions.hpp>
#include <VKThrowMacros.hpp>

#include <Terra.hpp>

DepthBuffer::DepthBuffer(
	VkDevice logicalDevice, std::vector<std::uint32_t> queueFamilyIndices
) : m_depthImage(VK_NULL_HANDLE), m_depthImageView(VK_NULL_HANDLE),
	m_deviceRef(logicalDevice), m_queueFamilyIndices(std::move(queueFamilyIndices)),
	m_memoryOffset{ 0u }, m_maxWidth{ 0u }, m_maxHeight{ 0u } {}

DepthBuffer::~DepthBuffer() noexcept {
	CleanUp(m_deviceRef);
}

void DepthBuffer::CleanUp(VkDevice device) noexcept {
	vkDestroyImageView(device, m_depthImageView, nullptr);
	vkDestroyImage(device, m_depthImage, nullptr);
}

DepthBuffer::DepthBuffer(DepthBuffer&& depthBuffer) noexcept
	: m_depthImage{ depthBuffer.m_depthImage },
	m_depthImageView{ depthBuffer.m_depthImageView }, m_deviceRef{ depthBuffer.m_deviceRef },
	m_queueFamilyIndices{ std::move(depthBuffer.m_queueFamilyIndices) },
	m_memoryOffset{ depthBuffer.m_memoryOffset }, m_maxWidth{ depthBuffer.m_maxWidth },
	m_maxHeight{ depthBuffer.m_maxHeight } {

	depthBuffer.m_depthImage = VK_NULL_HANDLE;
	depthBuffer.m_depthImageView = VK_NULL_HANDLE;
}
DepthBuffer& DepthBuffer::operator=(DepthBuffer&& depthBuffer) noexcept {
	m_depthImage = depthBuffer.m_depthImage;
	m_depthImageView = depthBuffer.m_depthImageView;
	m_deviceRef = depthBuffer.m_deviceRef;
	m_queueFamilyIndices = std::move(depthBuffer.m_queueFamilyIndices);
	m_memoryOffset = depthBuffer.m_memoryOffset;
	m_maxWidth = depthBuffer.m_maxWidth;
	m_maxHeight = depthBuffer.m_maxHeight;

	depthBuffer.m_depthImage = VK_NULL_HANDLE;
	depthBuffer.m_depthImageView = VK_NULL_HANDLE;

	return *this;
}

void DepthBuffer::CreateDepthImage(
	VkDevice device, VkImage* image,
	std::uint32_t width, std::uint32_t height,
	VkFormat depthFormat
) const {
	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.depth = 1u;
	createInfo.extent.height = height;
	createInfo.extent.width = width;
	createInfo.mipLevels = 1u;
	createInfo.arrayLayers = 1u;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.flags = 0u;
	createInfo.format = depthFormat;

	ConfigureImageQueueAccess(m_queueFamilyIndices, createInfo);

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateImage(device, &createInfo, nullptr, image)
	);
}

void DepthBuffer::AllocateForMaxResolution(
	VkDevice device, std::uint32_t width, std::uint32_t height
) {
	VkImage maxResImage = VK_NULL_HANDLE;
	CreateDepthImage(device, &maxResImage, width, height, DEPTHFORMAT);

	VkMemoryRequirements memoryReq{};
	vkGetImageMemoryRequirements(device, maxResImage, &memoryReq);

	vkDestroyImage(device, maxResImage, nullptr);

	if (!Terra::Resources::gpuOnlyMemory->CheckMemoryType(memoryReq))
		VK_GENERIC_THROW("Memory Type doesn't match with Depth buffer requirements.");

	m_memoryOffset = Terra::Resources::gpuOnlyMemory->ReserveSizeAndGetOffset(memoryReq);

	m_maxWidth = width;
	m_maxHeight = height;
}

void DepthBuffer::CreateDepthBuffer(
	VkDevice device, std::uint32_t width, std::uint32_t height
) {
	if (width > m_maxWidth || height > m_maxHeight)
		VK_GENERIC_THROW("Depth buffer resolution exceeds max resolution");

	CreateDepthImage(device, &m_depthImage, width, height, DEPTHFORMAT);

	VkDeviceMemory memoryStart = Terra::Resources::gpuOnlyMemory->GetMemoryHandle();

	VkResult result;
	VK_THROW_FAILED(result,
		vkBindImageMemory(
			device, m_depthImage, memoryStart, m_memoryOffset
		)
	);

	CreateImageView(
		device, m_depthImage, &m_depthImageView,
		DEPTHFORMAT, VK_IMAGE_ASPECT_DEPTH_BIT
	);
}

VkFormat DepthBuffer::GetDepthFormat() const noexcept {
	return DEPTHFORMAT;
}

VkImageView DepthBuffer::GetDepthImageView() const noexcept {
	return m_depthImageView;
}
