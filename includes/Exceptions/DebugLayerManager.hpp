#ifndef __DEBUG_LAYER_MANAGER_HPP__
#define __DEBUG_LAYER_MANAGER_HPP__
#include <vulkan/vulkan.h>

class DebugLayerManager {
public:
	DebugLayerManager(VkInstance instanceRef);
	~DebugLayerManager() noexcept;

	static VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger
	) noexcept;

	static void DestroyDebugUtilsMessengerEXT(
		VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator
	) noexcept;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);

	static void PopulateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& createInfo
	) noexcept;

private:
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkInstance m_pInstanceRef;
};

DebugLayerManager* CreateDebugLayerInstance(VkInstance instanceRef);

#endif
