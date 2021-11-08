#include <DebugLayerManager.hpp>
#include <iostream>
#include <VKThrowMacros.hpp>

DebugLayerManager::DebugLayerManager(VkInstance instanceRef)
	: m_pInstanceRef(instanceRef) {

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	VkResult result;
	VK_THROW_FAILED(result,
		CreateDebugUtilsMessengerEXT(
			m_pInstanceRef, &createInfo, nullptr, &m_debugMessenger
		)
	);
}

DebugLayerManager::~DebugLayerManager() noexcept {
	if(m_pInstanceRef)
		DestroyDebugUtilsMessengerEXT(m_pInstanceRef, m_debugMessenger, nullptr);
}

VkResult DebugLayerManager::CreateDebugUtilsMessengerEXT(
	VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) const noexcept {
	PFN_vkCreateDebugUtilsMessengerEXT function =
		reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>
		(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (function)
		return function(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DebugLayerManager::DestroyDebugUtilsMessengerEXT(
	VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator
) const noexcept {
	PFN_vkDestroyDebugUtilsMessengerEXT function =
		reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>
		(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (function)
		return function(instance, debugMessenger, pAllocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugLayerManager::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void DebugLayerManager::PopulateDebugMessengerCreateInfo(
	VkDebugUtilsMessengerCreateInfoEXT& createInfo
) noexcept {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

