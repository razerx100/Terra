#include <DebugLayerManager.hpp>
#include <fstream>
#include <unordered_map>
#include <array>

static constexpr std::array validationLayersNames
{
	"VK_LAYER_KHRONOS_validation"
};

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

DebugLayerManager::DebugLayerManager()
	: m_debugMessengers{}, m_callbackTypes{}, m_pInstanceRef{VK_NULL_HANDLE}, m_layers{}
{
	AddValidationLayer(ValidationLayer::VkLayerKhronosValidation);
}

void DebugLayerManager::DestroyDebugCallbacks() noexcept
{
	using DebugUtil = VkInstanceExtension::VkExtDebugUtils;


	for (VkDebugUtilsMessengerEXT debugMessenger : m_debugMessengers)
		DebugUtil::vkDestroyDebugUtilsMessengerEXT(m_pInstanceRef, debugMessenger, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugLayerManager::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*/* pUserData */
) {
	std::ofstream log("ErrorLog.txt", std::ios_base::app | std::ios_base::out);
	log << "Type : " << GenerateMessageType(messageType) << "    "
		<< "Severity : " << messageSeverities[messageSeverity] << "    "
		<< "ID : " << pCallbackData->pMessageIdName << "\n"
		<< "Description : " << pCallbackData->pMessage << "    "
		<< std::endl;

	return VK_FALSE;
}

void DebugLayerManager::CreateDebugCallbacks(VkInstance instance)
{
	m_pInstanceRef = instance;

	using DebugUtil = VkInstanceExtension::VkExtDebugUtils;

	for (DebugCallbackType type : m_callbackTypes)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = GetDebugCallbackMessengerCreateInfo();

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

		DebugUtil::vkCreateDebugUtilsMessengerEXT(
			m_pInstanceRef, &createInfo, nullptr, &debugMessenger
		);

		m_debugMessengers.emplace_back(debugMessenger);
	}
}

void DebugLayerManager::AddDebugCallback(DebugCallbackType type) noexcept
{
	m_callbackTypes.emplace_back(type);
}

void DebugLayerManager::AddValidationLayer(ValidationLayer layer) noexcept
{
	m_layers.emplace_back(validationLayersNames.at(static_cast<size_t>(layer)));
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

VkDebugUtilsMessengerCreateInfoEXT DebugLayerManager::GetDebugCallbackMessengerCreateInfo(
) const noexcept {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = DebugLayerManager::DebugCallback
	};

	return createInfo;
}

bool DebugLayerManager::CheckLayerSupport() const noexcept
{
	using namespace std::string_literals;

	std::uint32_t layerCount = 0u;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(
		&layerCount, std::data(availableLayers)
	);

	for (const char* requiredLayer : m_layers)
	{
		bool found = false;
		for (const VkLayerProperties& layer : availableLayers)
			if (std::strcmp(requiredLayer, layer.layerName) == 0)
			{
				found = true;
				break;
			}

		if (!found)
			return false;
	}

	return true;
}

