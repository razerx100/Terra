#ifndef DISPLAY_MANAGER_HPP_
#define DISPLAY_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <VkExtensionManager.hpp>

class DisplayInstanceExtension
{
public:
	virtual ~DisplayInstanceExtension() = default;

	virtual void SetInstanceExtensions(VkInstanceExtensionManager& extensionManager) noexcept = 0;
};

class DisplayManager
{
public:
	struct Resolution
	{
		std::uint32_t width;
		std::uint32_t height;
	};

	virtual ~DisplayManager() = default;

	[[nodiscard]]
	virtual Resolution GetDisplayResolution(VkPhysicalDevice gpu, std::uint32_t displayIndex) const = 0;
};
#endif
