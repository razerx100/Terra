#ifndef I_DISPLAY_MANAGER_HPP_
#define I_DISPLAY_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkExtensionManager.hpp>

class IDisplayManager
{
public:
	using Resolution = std::pair<std::uint64_t, std::uint64_t>;

	virtual ~IDisplayManager() = default;

	[[nodiscard]]
	virtual const std::vector<InstanceExtension>& GetRequiredExtensions() const noexcept = 0;
	[[nodiscard]]
	virtual Resolution GetDisplayResolution(
		VkPhysicalDevice gpu, std::uint32_t displayIndex
	) const = 0;
};
#endif
