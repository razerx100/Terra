#include <VkTextureView.hpp>

float VkSamplerCreateInfoBuilder::s_maxAnisotropy = 1.f;

VkSamplerCreateInfoBuilder::VkSamplerCreateInfoBuilder()
	: m_createInfo{
		.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.minFilter               = VK_FILTER_LINEAR,
		.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias              = 0.f,
		.anisotropyEnable        = VK_FALSE,
		.maxAnisotropy           = 1.f,
		.compareEnable           = VK_FALSE,
		.compareOp               = VK_COMPARE_OP_ALWAYS,
		.minLod                  = 0.f,
		.maxLod                  = 0.f,
		.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	}
{}

void VkSamplerCreateInfoBuilder::SetMaxAnisotropy(VkPhysicalDevice physicalDevice) noexcept
{
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	s_maxAnisotropy = properties.limits.maxSamplerAnisotropy;
}

VkSamplerCreateInfoBuilder& VkSamplerCreateInfoBuilder::Anisotropy(float anisotropy) noexcept
{
	m_createInfo.anisotropyEnable = VK_TRUE;
	m_createInfo.maxAnisotropy    = s_maxAnisotropy >= anisotropy ? anisotropy : s_maxAnisotropy;

	return *this;
}

// Sampler

VKSampler::~VKSampler() noexcept
{
	vkDestroySampler(m_device, m_sampler, nullptr);
}

void VKSampler::CreateSampler(const VkSamplerCreateInfo* createInfo)
{
	vkCreateSampler(m_device, createInfo, nullptr, &m_sampler);
}

void VKSampler::CreateSampler(const VkSamplerCreateInfo& createInfo)
{
	CreateSampler(&createInfo);
}

// Texture View
VkTextureView::~VkTextureView() noexcept
{
	vkDestroyImageView(m_device, m_imageView, nullptr);
}

void VkTextureView::CreateView(
	std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
	VkImageUsageFlags textureUsageFlags, VkImageAspectFlags aspectFlags,
	VkImageViewType imageType, std::uint32_t mipLevel, std::uint32_t mipLevelCount,
	const std::vector<std::uint32_t>& queueFamilyIndices
) {
	m_texture.Create(width, height, imageFormat, textureUsageFlags, queueFamilyIndices);
	CreateImageView(aspectFlags, imageType, mipLevel, mipLevelCount);
}

void VkTextureView::CreateImageView(
	VkImageAspectFlags aspectFlags, VkImageViewType imageType,
	std::uint32_t mipLevel, std::uint32_t mipLevelCount
) {
	VkComponentMapping componentMapping{
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY
	};
	VkImageSubresourceRange subresourceRange{
		.aspectMask     = aspectFlags,
		.baseMipLevel   = mipLevel,
		.levelCount     = mipLevelCount,
		.baseArrayLayer = 0u,
		.layerCount     = 1u
	};

	VkImageViewCreateInfo createInfo{
		.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image            = m_texture.Get(),
		.viewType         = imageType,
		.format           = m_texture.Format(),
		.components       = componentMapping,
		.subresourceRange = subresourceRange
	};

	vkCreateImageView(m_device, &createInfo, nullptr, &m_imageView);
}
