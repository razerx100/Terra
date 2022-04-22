#include <ObjectCreationFunctions.hpp>
#include <VKThrowMacros.hpp>

void CreateImageView(
	VkDevice device, VkImage image, VkImageView* imageView,
	VkFormat imageFormat
) {
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = imageFormat;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0u;
	createInfo.subresourceRange.levelCount = 1u;
	createInfo.subresourceRange.baseArrayLayer = 0u;
	createInfo.subresourceRange.layerCount = 1u;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateImageView(device, &createInfo, nullptr, imageView)
	);
}

void CreateSampler(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	VkSampler* sampler,
	bool anisotropy, float maxAnisotropy
) {
	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.f;
	createInfo.minLod = 0.f;
	createInfo.maxLod = 0.f;

	if (anisotropy) {
		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		const float deviceMaxAnisotrophy = properties.limits.maxSamplerAnisotropy;

		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy =
			(maxAnisotropy != 1.f && deviceMaxAnisotrophy >= maxAnisotropy ?
			maxAnisotropy : deviceMaxAnisotrophy);
	}
	else {
		createInfo.anisotropyEnable = VK_FALSE;
		createInfo.maxAnisotropy = maxAnisotropy;
	}

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateSampler(logicalDevice, &createInfo, nullptr, sampler)
	);
}
