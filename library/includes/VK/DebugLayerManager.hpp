#ifndef DEBUG_LAYER_MANAGER_HPP_
#define DEBUG_LAYER_MANAGER_HPP_
#include <vulkan/vulkan.h>
#include <string>
#include <VkExtensionManager.hpp>

void PopulateDebugMessengerCreateInfo(
	VkDebugUtilsMessengerCreateInfoEXT& createInfo
) noexcept;

class DebugLayerManager
{
public:
	DebugLayerManager(VkInstance instance);
	~DebugLayerManager() noexcept;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);

private:
	static std::string GenerateMessageType(std::uint32_t typeFlag) noexcept;

private:
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkInstance               m_pInstanceRef;

	static constexpr std::array s_requiredExtensions
	{
		InstanceExtension::VkExtDebugUtils
	};

public:
	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{ return s_requiredExtensions; }
};
#endif
