#ifndef VK_INSTANCE_MANAGER_HPP_
#define VK_INSTANCE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <optional>
#include <utility>
#include <VkFeatureManager.hpp>
#include <VkDebugLayerManager.hpp>

class VkInstanceManager
{
public:
	VkInstanceManager(std::string_view appName);
	~VkInstanceManager() noexcept;

	void CreateInstance(CoreVersion version);

	[[nodiscard]]
	VkInstanceExtensionManager& ExtensionManager() noexcept { return m_extensionManager; }
	[[nodiscard]]
	DebugLayerManager& DebugLayers() noexcept { return m_debugLayer; }

	[[nodiscard]]
	VkInstance GetVKInstance() const noexcept;
	[[nodiscard]]
	CoreVersion GetCoreVersion() const noexcept;

private:
	[[nodiscard]]
	// Returns empty if all of the extensions are supported. If an extension isn't supported
	// returns its name.
	std::optional<std::string_view> CheckExtensionSupport() const noexcept;

	[[nodiscard]]
	static std::uint32_t GetCoreVersion(CoreVersion version) noexcept;

	void SelfDestruct() noexcept;

private:
	VkInstance                 m_vkInstance;
	std::string_view           m_appName;
	CoreVersion                m_coreVersion;
	VkInstanceExtensionManager m_extensionManager;
	DebugLayerManager          m_debugLayer;

public:
	VkInstanceManager(const VkInstanceManager&) = delete;
	VkInstanceManager& operator=(const VkInstanceManager&) = delete;

	VkInstanceManager(VkInstanceManager&& other) noexcept
		: m_vkInstance{ std::exchange(other.m_vkInstance, VK_NULL_HANDLE) },
		m_appName{ std::move(other.m_appName) },
		m_coreVersion{ other.m_coreVersion }, m_extensionManager{ std::move(other.m_extensionManager) },
		m_debugLayer{ std::move(other.m_debugLayer) }
	{}

	VkInstanceManager& operator=(VkInstanceManager&& other) noexcept
	{
		SelfDestruct();

		m_vkInstance       = std::exchange(other.m_vkInstance, VK_NULL_HANDLE);
		m_appName          = std::move(other.m_appName);
		m_coreVersion      = other.m_coreVersion;
		m_extensionManager = std::move(other.m_extensionManager);
		m_debugLayer       = std::move(other.m_debugLayer);

		return *this;
	}
};
#endif
