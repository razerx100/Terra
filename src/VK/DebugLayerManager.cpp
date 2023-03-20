#include <DebugLayerManager.hpp>
#include <fstream>
#include <unordered_map>

static std::unordered_map<VkDebugUtilsMessageSeverityFlagBitsEXT, const char*> messageSeverities{
	{VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, "MESSAGE_SEVERITY_VERBOSE" },
	{VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, "MESSAGE_SEVERITY_INFO"},
	{VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, "MESSAGE_SEVERITY_WARNING"},
	{VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, "MESSAGE_SEVERITY_ERROR"},
	{VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT, "NOTHING"}
};

static std::unordered_map<std::uint32_t, const char*> messageTypes{
	{VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, "MESSAGE_TYPE_GENERAL"},
	{VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "MESSAGE_TYPE_VALIDATION"},
	{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "MESSAGE_TYPE_PERFORMANCE"},
	{VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT, "NOTHING"}
};

DebugLayerManager::DebugLayerManager(const Args& arguments)
	: m_debugMessenger{ VK_NULL_HANDLE }, m_pInstanceRef{ arguments.instance.value() } {

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	CreateDebugUtilsMessengerEXT(m_pInstanceRef, &createInfo, nullptr, &m_debugMessenger);
}

DebugLayerManager::~DebugLayerManager() noexcept {
	DestroyDebugUtilsMessengerEXT(m_pInstanceRef, m_debugMessenger, nullptr);
}

VkResult DebugLayerManager::CreateDebugUtilsMessengerEXT(
	VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) noexcept {
	auto function =
		reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>
		(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (function)
		return function(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DebugLayerManager::DestroyDebugUtilsMessengerEXT(
	VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator
) noexcept {
	auto function =
		reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>
		(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (function)
		function(instance, debugMessenger, pAllocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugLayerManager::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData
) {
	std::ofstream log("ErrorLog.txt", std::ios_base::app | std::ios_base::out);
	log << "Type : " << GenerateMessageType(messageType) << "    "
		<< "Severity : " << messageSeverities[messageSeverity] << "    "
		<< "ID : " << pCallbackData->pMessageIdName << "\n"
		<< "Description : " << pCallbackData->pMessage << "    "
		<< std::endl;

	return VK_FALSE;
}

std::string DebugLayerManager::GenerateMessageType(std::uint32_t typeFlag) noexcept {
	std::string messageTypeDescription;
	for (std::uint32_t index = 0u; index < 3u; ++index) {
		const std::uint32_t flagIndex = 1u << index;
		if (typeFlag & flagIndex)
			messageTypeDescription
			.append(messageTypes[static_cast<size_t>(flagIndex)])
			.append(" ");
	}

	return messageTypeDescription;
}

void PopulateDebugMessengerCreateInfo(
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
	createInfo.pfnUserCallback = DebugLayerManager::DebugCallback;
}

