#ifndef VK_INSTANCE_MANAGER_HPP_
#define VK_INSTANCE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <optional>

class VkInstanceManager {
public:
	struct Args {
		std::optional<std::string> appName;
	};

public:
	VkInstanceManager(Args& arguments);
	~VkInstanceManager() noexcept;

	void AddExtensionNames(const std::vector<const char*>& extensionNames) noexcept;
	void CreateInstance();

	[[nodiscard]]
	VkInstance GetVKInstance() const noexcept;

private:
	void CheckExtensionSupport() const;
	void CheckLayerSupport() const;

private:
	VkInstance m_vkInstance;
	std::string m_appName;

	std::vector<const char*> m_extensionNames = {
		"VK_KHR_surface"
	};
	std::vector<const char*> m_validationLayersNames = {
		"VK_LAYER_KHRONOS_validation"
	};
};
#endif
