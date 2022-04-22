#ifndef OBJECT_CREATION_FUNCTIONS_
#define OBJECT_CREATION_FUNCTIONS_
#include <vulkan/vulkan.hpp>

void CreateImageView(
	VkDevice device, VkImage image, VkImageView* imageView,
	VkFormat imageFormat
);

void CreateSampler(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	VkSampler* sampler,
	bool anisotropy = false, float maxAnisotropy = 1.f
);
#endif
