#ifndef __DISPLAY_MANAGER_VK_HPP__
#define __DISPLAY_MANAGER_VK_HPP__
#include <IDisplayManager.hpp>

class DisplayManagerVK : public IDisplayManager {
public:
	void InitDisplayManager(VkPhysicalDevice gpu) override;

	const std::vector<const char*>& GetRequiredExtensions() const noexcept override;
	void GetDisplayResolution(VkPhysicalDevice gpu, SRect& displayRect) override;

private:
	const std::vector<const char*> m_requiredExtensions = {
		"VK_KHR_display"
	};
};

#endif
