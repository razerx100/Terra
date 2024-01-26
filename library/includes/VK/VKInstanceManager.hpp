#ifndef VK_INSTANCE_MANAGER_HPP_
#define VK_INSTANCE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>

class VkInstanceManager
{
public:
	VkInstanceManager(std::string_view appName);
	~VkInstanceManager() noexcept;

	VkInstanceManager& AddExtensionNames(const std::vector<const char*>& extensionNames) noexcept;
	void CreateInstance();

	[[nodiscard]]
	VkInstance GetVKInstance() const noexcept;

private:
	void CheckExtensionSupport() const;
	void CheckLayerSupport() const;

private:
	VkInstance m_vkInstance;
	std::string_view m_appName;

	std::vector<const char*> m_extensionNames = {
		"VK_KHR_surface"
	};
	std::vector<const char*> m_validationLayersNames = {
		"VK_LAYER_KHRONOS_validation"
	};
};
#endif
