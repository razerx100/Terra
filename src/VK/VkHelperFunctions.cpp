#include <VkHelperFunctions.hpp>
#include <VKThrowMacros.hpp>

void CreateImageView(
	VkDevice device, VkImage image, VkImageView* imageView,
	VkFormat imageFormat, VkImageAspectFlags aspectFlags
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
	createInfo.subresourceRange.aspectMask = aspectFlags;
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

std::optional<FamilyInfo> QueryQueueFamilyInfo(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept {
	std::uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
		device, &queueFamilyCount, std::data(queueFamilies)
	);

	bool transfer = false;
	bool compute = false;
	bool graphics = false;

	FamilyInfo familyInfo;

	// Transfer only
	for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
		VkQueueFamilyProperties& queueFamily = queueFamilies[index];

		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT
			&& !(queueFamily.queueFlags & 3u)) {
			familyInfo.emplace_back(index, QueueType::TransferQueue);
			transfer = true;
			--queueFamily.queueCount;

			break;
		}
	}

	if (transfer)
		for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];

			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT
				&& !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& queueFamily.queueCount) {
				familyInfo.emplace_back(index, QueueType::ComputeQueue);
				compute = true;
				--queueFamily.queueCount;

				break;
			}
		}
	else
		for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];

			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT
				&& !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				&& queueFamily.queueCount >= 2) {
				familyInfo.emplace_back(index, QueueType::TransferQueue);
				familyInfo.emplace_back(index, QueueType::ComputeQueue);
				compute = true;
				transfer = true;
				queueFamily.queueCount -= 2;

				break;
			}
		}

	if (!transfer)
		for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];

			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT
				&& queueFamily.queueCount) {
				familyInfo.emplace_back(index, QueueType::TransferQueue);
				transfer = true;
				--queueFamily.queueCount;

				break;
			}
		}

	if (!compute)
		for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];

			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT
				&& queueFamily.queueCount) {
				familyInfo.emplace_back(index, QueueType::ComputeQueue);
				compute = true;
				--queueFamily.queueCount;

				break;
			}
		}

	for (size_t index = 0u; index < std::size(queueFamilies); ++index) {
			VkQueueFamilyProperties& queueFamily = queueFamilies[index];

		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT
			&& CheckPresentSupport(device, surface, index)
			&& queueFamily.queueCount >= 1) {
			familyInfo.emplace_back(index, QueueType::GraphicsQueue);
			graphics = true;
			--queueFamily.queueCount;

			break;
		}
	}

	if (graphics && compute && transfer)
		return familyInfo;
	else
		return {};
}

bool CheckPresentSupport(
	VkPhysicalDevice device, VkSurfaceKHR surface, size_t index
) noexcept {
	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(
		device, static_cast<std::uint32_t>(index), surface, &presentSupport
	);

	return presentSupport;
}

SurfaceInfo QuerySurfaceCapabilities(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept {
	SurfaceInfo surfaceInfo = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceInfo.capabilities);

	std::uint32_t formatCount = 0u;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount) {
		surfaceInfo.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device, surface, &formatCount, std::data(surfaceInfo.formats)
		);
	}

	std::uint32_t presentModeCount = 0u;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount) {
		surfaceInfo.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, surface, &presentModeCount, std::data(surfaceInfo.presentModes)
		);
	}

	return surfaceInfo;
}

void ConfigureImageQueueAccess(
	const std::vector<std::uint32_t>& queueFamilyIndices,
	VkImageCreateInfo& imageInfo
) noexcept {
	if (std::size(queueFamilyIndices) > 1u) {
		imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		imageInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(
			std::size(queueFamilyIndices)
			);
		imageInfo.pQueueFamilyIndices = std::data(queueFamilyIndices);
	}
	else
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
}

size_t FindMemoryType(
	VkPhysicalDevice physicalDevice,
	const VkMemoryRequirements& memoryReq, VkMemoryPropertyFlags propertiesToCheck
) noexcept {
	VkPhysicalDeviceMemoryProperties memoryProp = {};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProp);

	for(size_t index = 0u; index < memoryProp.memoryTypeCount; ++index)
		if ((memoryReq.memoryTypeBits & (1u << index))
			&& (memoryProp.memoryTypes[index].propertyFlags & propertiesToCheck)
			== propertiesToCheck) {

			return index;
		}

	return 0u;
}
