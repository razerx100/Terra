#ifndef DISPLAY_MANAGER_VK_HPP_
#define DISPLAY_MANAGER_VK_HPP_
#include <IDisplayManager.hpp>

class DisplayManagerVK final : public IDisplayManager {
public:
	[[nodiscard]]
	Resolution GetDisplayResolution(
		VkPhysicalDevice gpu, std::uint32_t displayIndex
	) const override;

private:
	inline static const std::vector<InstanceExtension> s_requiredExtensions
	{
		InstanceExtension::VkKhrDisplay
	};

public:
	[[nodiscard]]
	const std::vector<InstanceExtension>& GetRequiredExtensions() const noexcept override
	{ return s_requiredExtensions; }
};
#endif
