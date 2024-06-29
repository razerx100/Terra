#ifndef DISPLAY_MANAGER_VK_HPP_
#define DISPLAY_MANAGER_VK_HPP_
#include <DisplayManager.hpp>

class DisplayInstanceExtensionVk : public DisplayInstanceExtension
{
public:
	void SetInstanceExtensions(VkInstanceExtensionManager& extensionManager) noexcept override;
};

class DisplayManagerVK final : public DisplayManager
{
public:
	[[nodiscard]]
	Resolution GetDisplayResolution(VkPhysicalDevice gpu, std::uint32_t displayIndex) const override;
};
#endif
