#ifndef DISPLAY_MANAGER_VK_HPP_
#define DISPLAY_MANAGER_VK_HPP_
#include <IDisplayManager.hpp>

class DisplayManagerVK final : public IDisplayManager {
public:
	[[nodiscard]]
	const std::vector<const char*>& GetRequiredExtensions() const noexcept override;
	[[nodiscard]]
	Resolution GetDisplayResolution(
		VkPhysicalDevice gpu, std::uint32_t displayIndex
	) const override;

private:
	const std::vector<const char*> m_requiredExtensions = {
		"VK_KHR_display"
	};
};

#endif
