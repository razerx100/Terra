#ifndef __INATANCE_MANAGER_HPP__
#define __INATANCE_MANAGER_HPP__
#include <IInstanceManager.hpp>
#include <vector>

class InstanceManager : public IInstanceManager {
public:
	InstanceManager(const char* appName);
	~InstanceManager() noexcept;

	VkInstance GetVKInstance() const noexcept override;

private:
	void CheckExtensionSupport() const;
	void CheckLayerSupport() const;

private:
	VkInstance m_vkInstance;

	std::vector<const char*> m_extensionNames = {
		"VK_KHR_surface",
		"VK_KHR_display"
	};
	std::vector<const char*> m_validationLayersNames = {
		"VK_LAYER_KHRONOS_validation"
	};
};
#endif
