#include <VkHelperFunctions.hpp>

void CreateSampler(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkSampler* sampler,
	bool anisotropy, float maxAnisotropy
) {
	VkSamplerCreateInfo createInfo{};
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
		VkPhysicalDeviceProperties properties{};
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

	vkCreateSampler(logicalDevice, &createInfo, nullptr, sampler);
}

SurfaceInfo QuerySurfaceCapabilities(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept {
	SurfaceInfo surfaceInfo{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceInfo.capabilities);

	std::uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	surfaceInfo.formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		device, surface, &formatCount, std::data(surfaceInfo.formats)
	);

	std::uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	surfaceInfo.presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device, surface, &presentModeCount, std::data(surfaceInfo.presentModes)
	);

	return surfaceInfo;
}

std::vector<std::uint32_t> ResolveQueueIndices(
	std::uint32_t index0, std::uint32_t index1, std::uint32_t index2
) noexcept {
	std::vector<uint32_t> distinctQueueIndices{ index0, index1, index2 };
	std::ranges::sort(distinctQueueIndices);

	auto ret = std::ranges::unique(distinctQueueIndices);
	distinctQueueIndices.erase(std::begin(ret), std::end(ret));

	return distinctQueueIndices;
}

std::vector<std::uint32_t> ResolveQueueIndices(
	std::uint32_t index0, std::uint32_t index1
) noexcept {
	return ResolveQueueIndices(index0, index1, index0);
}

