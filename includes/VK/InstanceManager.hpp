#ifndef __INATANCE_MANAGER_HPP__
#define __INATANCE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>

class InstanceManager {
public:
	InstanceManager(const char* appName);
	~InstanceManager() noexcept;

	VkInstance GetInstanceRef() const noexcept;

private:
	void CheckExtensionSupport() const;
	void CheckLayerSupport() const;

private:
	VkInstance m_vkInstance;

	std::vector<const char*> m_extensionNames = {};
	std::vector<const char*> m_validationLayersNames = {
		"VK_LAYER_KHRONOS_validation"
	};
};

InstanceManager* GetVKInstance() noexcept;
void InitVKInstance(const char* appName);
void CleanUpVKInstance() noexcept;
#endif
