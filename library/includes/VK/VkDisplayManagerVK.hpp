#ifndef VK_DISPLAY_MANAGER_VK_HPP_
#define VK_DISPLAY_MANAGER_VK_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <VkExtensionManager.hpp>

namespace Terra
{
namespace DisplayInstanceExtensionVk
{
	void SetInstanceExtensions(VkInstanceExtensionManager& extensionManager) noexcept;
};

class DisplayManagerVK
{
public:
	[[nodiscard]]
	VkExtent2D GetDisplayResolution(VkPhysicalDevice gpu, std::uint32_t displayIndex) const;
};
}
#endif
