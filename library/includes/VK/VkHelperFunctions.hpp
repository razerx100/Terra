#ifndef VK_HELPER_FUNCTIONS_
#define VK_HELPER_FUNCTIONS_
#include <vulkan/vulkan.hpp>
#include <optional>
#include <concepts>
#include <vector>

struct SurfaceInfo {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	[[nodiscard]]
	bool IsCapable() const noexcept {
		return !formats.empty() && !presentModes.empty();
	}
};

void CreateSampler(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkSampler* sampler,
	bool anisotropy = false, float maxAnisotropy = 1.f
);

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

[[nodiscard]]
std::vector<std::uint32_t> ResolveQueueIndices(
	std::uint32_t index0, std::uint32_t index1, std::uint32_t index2
) noexcept;

[[nodiscard]]
std::vector<std::uint32_t> ResolveQueueIndices(
	std::uint32_t index0, std::uint32_t index1
) noexcept;
#endif
