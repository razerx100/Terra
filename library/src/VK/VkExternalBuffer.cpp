#include <VkExternalBuffer.hpp>
#include <VkExternalFormatMap.hpp>

// External Buffer
VkExternalBuffer::VkExternalBuffer(
	VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType,
	VkBufferUsageFlags usageFlags
) : m_buffer{ device, memoryManager, memoryType }, m_usageFlags{ usageFlags }
{}

void VkExternalBuffer::Create(size_t bufferSize)
{
	// For now, let's assume these buffers would only be used in the Graphics queue.
	m_buffer.Create(static_cast<VkDeviceSize>(bufferSize), m_usageFlags, {});
}

// External Texture
VkExternalTexture::VkExternalTexture(VkDevice device, MemoryManager* memoryManager)
	: m_textureView{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_currentAccessState{ VK_ACCESS_NONE }, m_currentLayoutState{ VK_IMAGE_LAYOUT_UNDEFINED },
	m_currentPipelineStage{ VK_PIPELINE_STAGE_NONE }
{}

void VkExternalTexture::Destroy() noexcept
{
	m_textureView.Destroy();

	m_currentAccessState = VK_ACCESS_NONE;
	m_currentLayoutState = VK_IMAGE_LAYOUT_UNDEFINED;
}

bool VkExternalTexture::DoesContainFlag(
	ExternalTextureCreationFlagBit flagBit, std::uint32_t flag
) noexcept {
	const auto uFlagBit = static_cast<std::uint32_t>(flagBit);

	return (flag & uFlagBit) == uFlagBit;
}

void VkExternalTexture::Create(
	std::uint32_t width, std::uint32_t height, ExternalFormat format, ExternalTexture2DType type,
	std::uint32_t creationFlags
) {
	VkImageUsageFlags usageFlags   = 0u;
	VkImageAspectFlags aspectFlags = 0u;

	if (type == ExternalTexture2DType::RenderTarget)
	{
		usageFlags  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (type == ExternalTexture2DType::Depth)
	{
		usageFlags  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else if (type == ExternalTexture2DType::Stencil)
	{
		usageFlags  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectFlags = VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	if (DoesContainFlag(ExternalTextureCreationFlagBit::CopySrc, creationFlags))
		usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (DoesContainFlag(ExternalTextureCreationFlagBit::CopyDst, creationFlags))
		usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (DoesContainFlag(ExternalTextureCreationFlagBit::SampleTexture, creationFlags))
		usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;

	// For now, let's assume these textures would only be used in the Graphics queue.
	m_textureView.CreateView2D(
		width, height, GetVkFormat(format), usageFlags, aspectFlags, VK_IMAGE_VIEW_TYPE_2D, {}
	);
}

ImageBarrierBuilder VkExternalTexture::TransitionState(
	VkAccessFlagBits newAccess, VkImageLayout newLayout, VkPipelineStageFlagBits newStage
) noexcept {
	// From my current understanding, Vulkan doesn't do implicit layout/access transition like Dx12.
	// We can however define the previous layout as Undefined and that will make the stored data
	// undefined. So, when using one of these between frames, it should be fine to remember the
	// previous state and transition from that.
	ImageBarrierBuilder builder{};

	builder
		.Image(m_textureView)
		.AccessMasks(m_currentAccessState, newAccess)
		.StageMasks(m_currentPipelineStage, newStage)
		.Layouts(m_currentLayoutState, newLayout);

	m_currentAccessState   = newAccess;
	m_currentLayoutState   = newLayout;
	m_currentPipelineStage = newStage;

	return builder;
}
