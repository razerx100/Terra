#ifndef __INATANCE_MANAGER_HPP__
#define __INATANCE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>

class InstanceManager {
public:
	InstanceManager(const char* appName);
	~InstanceManager() noexcept;

	VkInstance GetVKInstance() const noexcept;

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

InstanceManager* GetInstanceManagerInstance() noexcept;
void InitInstanceManagerInstance(const char* appName);
void CleanUpInstanceManagerInstance() noexcept;
#endif
