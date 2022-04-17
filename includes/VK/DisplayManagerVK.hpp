#ifndef DISPLAY_MANAGER_VK_HPP_
#define DISPLAY_MANAGER_VK_HPP_
#include <IDisplayManager.hpp>

class DisplayManagerVK : public IDisplayManager {
public:
	void InitDisplayManager(VkPhysicalDevice gpu) override;

	[[nodiscard]]
	const std::vector<const char*>& GetRequiredExtensions() const noexcept override;
	void GetDisplayResolution(VkPhysicalDevice gpu, Ceres::Rect& displayRect) override;

private:
	const std::vector<const char*> m_requiredExtensions = {
		"VK_KHR_display"
	};
};

#endif
