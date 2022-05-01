#ifndef VK_HELPER_FUNCTIONS_
#define VK_HELPER_FUNCTIONS_
#include <vulkan/vulkan.hpp>
#include <vector>

enum QueueType {
	TransferQueue = 1,
	ComputeQueue = 2,
	GraphicsQueue = 4,
	PresentQueue = 8
};

struct SurfaceInfo {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	[[nodiscard]]
	bool IsCapable() const noexcept {
		return !formats.empty() && !presentModes.empty();
	}
};

using FamilyInfo = std::vector<std::pair<size_t, QueueType>>;

void CreateImageView(
	VkDevice device, VkImage image, VkImageView* imageView,
	VkFormat imageFormat
);

void CreateSampler(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	VkSampler* sampler,
	bool anisotropy = false, float maxAnisotropy = 1.f
);

[[nodiscard]]
FamilyInfo QueryQueueFamilyInfo(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept;

[[nodiscard]]
bool CheckPresentSupport(
	VkPhysicalDevice device, VkSurfaceKHR surface,
	size_t index
) noexcept;

[[nodiscard]]
SurfaceInfo QuerySurfaceCapabilities(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept;
#endif
