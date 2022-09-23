#include <DepthBuffer.hpp>
#include <VkHelperFunctions.hpp>
#include <VKThrowMacros.hpp>

#include <Terra.hpp>

DepthBuffer::DepthBuffer(
	VkDevice device, std::vector<std::uint32_t> queueFamilyIndices
) : m_depthImage{ device }, m_deviceRef{ device },
	m_queueFamilyIndices{ std::move(queueFamilyIndices) }, m_maxWidth{ 0u },
	m_maxHeight{ 0u } {}

void DepthBuffer::CleanUp() noexcept {
	m_depthImage.CleanUpImageResourceView();
}

DepthBuffer::DepthBuffer(DepthBuffer&& depthBuffer) noexcept
	: m_depthImage{ std::move(depthBuffer.m_depthImage) },
	m_deviceRef{ depthBuffer.m_deviceRef },
	m_queueFamilyIndices{ std::move(depthBuffer.m_queueFamilyIndices) },
	m_maxWidth{ depthBuffer.m_maxWidth }, m_maxHeight{ depthBuffer.m_maxHeight } {}

DepthBuffer& DepthBuffer::operator=(DepthBuffer&& depthBuffer) noexcept {
	m_depthImage = std::move(depthBuffer.m_depthImage);
	m_deviceRef = depthBuffer.m_deviceRef;
	m_queueFamilyIndices = std::move(depthBuffer.m_queueFamilyIndices);
	m_maxWidth = depthBuffer.m_maxWidth;
	m_maxHeight = depthBuffer.m_maxHeight;

	return *this;
}

void DepthBuffer::AllocateForMaxResolution(
	VkDevice device, std::uint32_t width, std::uint32_t height
) {
	VkDeviceSize depthOffset = 0u;

	{
		VkImageResourceView maxResImage{ device };
		std::vector<std::uint32_t> queueIndices;

		maxResImage.CreateResource(
			device, width, height, DEPTHFORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			queueIndices
		);

		maxResImage.SetMemoryOffsetAndType(device);
		depthOffset = maxResImage.GetMemoryOffset();
	}

	m_depthImage.SetMemoryOffsetAndType(depthOffset, MemoryType::gpuOnly);

	m_maxWidth = width;
	m_maxHeight = height;
}

void DepthBuffer::CreateDepthBuffer(
	VkDevice device, std::uint32_t width, std::uint32_t height
) {
	if (width > m_maxWidth || height > m_maxHeight)
		VK_GENERIC_THROW("Depth buffer resolution exceeds max resolution");

	m_depthImage.CreateResource(
		device, width, height, DEPTHFORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_queueFamilyIndices
	);

	m_depthImage.BindResourceToMemory(device);
	m_depthImage.CreateImageView(device, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkFormat DepthBuffer::GetDepthFormat() const noexcept {
	return DEPTHFORMAT;
}

VkImageView DepthBuffer::GetDepthImageView() const noexcept {
	return m_depthImage.GetImageView();
}
