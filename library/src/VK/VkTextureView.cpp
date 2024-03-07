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
	SelfDestruct();
}

void VKSampler::SelfDestruct() noexcept
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

// VKImageView
VKImageView::~VKImageView() noexcept
{
	SelfDestruct();
}

void VKImageView::SelfDestruct() noexcept
{
	vkDestroyImageView(m_device, m_imageView, nullptr);
}

void VKImageView::CreateView(
	VkImage image, VkFormat imageFormat, VkImageAspectFlags aspectFlags, VkImageViewType imageType,
	std::uint32_t mipBaseLevel, std::uint32_t mipLevelCount
) {
	VkComponentMapping componentMapping{
		.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.a = VK_COMPONENT_SWIZZLE_IDENTITY
	};
	VkImageSubresourceRange subresourceRange{
		.aspectMask     = aspectFlags,
		.baseMipLevel   = mipBaseLevel,
		.levelCount     = mipLevelCount,
		.baseArrayLayer = 0u,
		.layerCount     = 1u
	};

	VkImageViewCreateInfo createInfo{
		.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image            = image,
		.viewType         = imageType,
		.format           = imageFormat,
		.components       = componentMapping,
		.subresourceRange = subresourceRange
	};

	vkCreateImageView(m_device, &createInfo, nullptr, &m_imageView);

	m_imageAspect   = aspectFlags;
	m_mipBaseLevel  = mipBaseLevel;
	m_mipLevelCount = mipLevelCount;
}

// Texture View
void VkTextureView::CreateView2D(
	std::uint32_t width, std::uint32_t height,
	VkFormat imageFormat, VkImageUsageFlags textureUsageFlags, VkImageAspectFlags aspectFlags,
	VkImageViewType imageType, const std::vector<std::uint32_t>& queueFamilyIndices,
	std::uint32_t mipBaseLevel /* = 0u */ , std::uint32_t mipLevelCount /* = 1u */
) {
	m_texture.Create2D(
		width, height, mipLevelCount, imageFormat, textureUsageFlags, queueFamilyIndices
	);
	m_imageView.CreateView(
		m_texture.Get(), imageFormat, aspectFlags, imageType, mipBaseLevel, mipLevelCount
	);
}

void VkTextureView::CreateView3D(
	std::uint32_t width, std::uint32_t height, std::uint32_t depth,
	VkFormat imageFormat, VkImageUsageFlags textureUsageFlags, VkImageAspectFlags aspectFlags,
	VkImageViewType imageType, const std::vector<std::uint32_t>& queueFamilyIndices,
	std::uint32_t mipBaseLevel /* = 0u */ , std::uint32_t mipLevelCount /* = 1u */
) {
	m_texture.Create3D(
		width, height, depth, mipLevelCount, imageFormat, textureUsageFlags, queueFamilyIndices
	);
	m_imageView.CreateView(
		m_texture.Get(), imageFormat, aspectFlags, imageType, mipBaseLevel, mipLevelCount
	);
}

void VkTextureView::CreateView(
	Texture&& texture,
	VkFormat imageFormat, VkImageAspectFlags aspectFlags, VkImageViewType imageType,
	std::uint32_t mipBaseLevel /* = 0u */ , std::uint32_t mipLevelCount /* = 1u */
) {
	m_texture = std::move(texture);
	m_imageView.CreateView(
		m_texture.Get(), imageFormat, aspectFlags, imageType, mipBaseLevel, mipLevelCount
	);
}
