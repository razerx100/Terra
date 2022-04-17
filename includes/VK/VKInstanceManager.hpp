#ifndef VK_INSTANCE_MANAGER_HPP_
#define VK_INSTANCE_MANAGER_HPP_
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
	void AddExtensionNames(const std::vector<const char*>& extensionNames) noexcept;

private:
	VkInstance m_vkInstance;

	std::vector<const char*> m_extensionNames = {
		"VK_KHR_surface"
	};
	std::vector<const char*> m_validationLayersNames = {
		"VK_LAYER_KHRONOS_validation"
	};
};
#endif
