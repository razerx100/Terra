#ifndef VK_HELPER_FUNCTIONS_
#define VK_HELPER_FUNCTIONS_
#include <vulkan/vulkan.hpp>
#include <optional>
#include <concepts>
#include <vector>

enum QueueType {
	TransferQueue = 1,
	ComputeQueue = 2,
	GraphicsQueue = 4
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
	VkFormat imageFormat, VkImageAspectFlags aspectFlags
);

void CreateSampler(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkSampler* sampler,
	bool anisotropy = false, float maxAnisotropy = 1.f
);

void ConfigureImageQueueAccess(
	const std::vector<std::uint32_t>& queueFamilyIndices,
	VkImageCreateInfo& imageInfo
) noexcept;

[[nodiscard]]
std::optional<FamilyInfo> QueryQueueFamilyInfo(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept;

[[nodiscard]]
bool CheckPresentSupport(
	VkPhysicalDevice device, VkSurfaceKHR surface, size_t index
) noexcept;

[[nodiscard]]
SurfaceInfo QuerySurfaceCapabilities(
	VkPhysicalDevice device, VkSurfaceKHR surface
) noexcept;

template<std::integral Integer>
[[nodiscard]]
constexpr Integer Align(Integer address, Integer alignment) noexcept {
	size_t _address = static_cast<size_t>(address);
	size_t _alignment = static_cast<size_t>(alignment);
	return static_cast<Integer>((_address + (_alignment - 1u)) & ~(_alignment - 1u));
}

constexpr size_t operator"" _B(unsigned long long number) noexcept {
	return static_cast<size_t>(number);
}

constexpr size_t operator"" _KB(unsigned long long number) noexcept {
	return static_cast<size_t>(number * 1024u);
}

constexpr size_t operator"" _MB(unsigned long long number) noexcept {
	return static_cast<size_t>(number * 1024u * 1024u);
}

constexpr size_t operator"" _GB(unsigned long long number) noexcept {
	return static_cast<size_t>(number * 1024u * 1024u * 1024u);
}
#endif
