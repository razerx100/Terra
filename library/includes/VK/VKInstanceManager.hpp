#ifndef VK_INSTANCE_MANAGER_HPP_
#define VK_INSTANCE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <VkFeatureManager.hpp>

class VkInstanceManager
{
public:
	VkInstanceManager(std::string_view appName);
	~VkInstanceManager() noexcept;

	VkInstanceManager& AddExtensionNames(const std::vector<const char*>& extensionNames) noexcept;
	void CreateInstance(CoreVersion version);

	[[nodiscard]]
	VkInstance GetVKInstance() const noexcept;
	[[nodiscard]]
	CoreVersion GetCoreVersion() const noexcept;

private:
	void CheckExtensionSupport() const;
	void CheckLayerSupport() const;

	[[nodiscard]]
	static std::uint32_t GetCoreVersion(CoreVersion version) noexcept;

private:
	VkInstance       m_vkInstance;
	std::string_view m_appName;
	CoreVersion      m_coreVersion;

	std::vector<const char*> m_extensionNames = {
		"VK_KHR_surface"
	};
	std::vector<const char*> m_validationLayersNames = {
		"VK_LAYER_KHRONOS_validation"
	};
};
#endif
