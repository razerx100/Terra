#include <VkDebugLayerManager.hpp>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <array>
#include <format>

namespace Terra
{
static constexpr std::array validationLayersNames
{
	"VK_LAYER_KHRONOS_validation"
};

static std::unordered_map<VkDebugUtilsMessageSeverityFlagBitsEXT, const char*> messageSeverities
{
	{ VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,        "MESSAGE_SEVERITY_VERBOSE" },
	{ VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,           "MESSAGE_SEVERITY_INFO"},
	{ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,        "MESSAGE_SEVERITY_WARNING"},
	{ VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,          "MESSAGE_SEVERITY_ERROR"},
	{ VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT, "NOTHING"}
};

static std::unordered_map<std::uint32_t, const char*> messageTypes
{
	{ VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,                "MESSAGE_TYPE_GENERAL"},
	{ VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,             "MESSAGE_TYPE_VALIDATION"},
	{ VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,            "MESSAGE_TYPE_PERFORMANCE"},
	{ VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT, "MESSAGE_TYPE_DEVICE_ADDRESS_BINDING"},
	{ VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT,         "NOTHING"}
};

DebugLayerManager::DebugLayerManager()
	: m_debugMessengers{}, m_callbackTypes{}, m_instance{ VK_NULL_HANDLE }, m_layers{}
{
	AddValidationLayer(ValidationLayer::VkLayerKhronosValidation);
}

void DebugLayerManager::DestroyDebugCallbacks() noexcept
{
	using DebugUtil = VkInstanceExtension::VkExtDebugUtils;

	for (VkDebugUtilsMessengerEXT debugMessenger : m_debugMessengers)
		DebugUtil::vkDestroyDebugUtilsMessengerEXT(m_instance, debugMessenger, nullptr);
}

std::string VKAPI_CALL DebugLayerManager::FormatDebugMessage(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
) {
	return std::format(
		"Type : {}    Severity : {}    ID : {}.\nDescription : {}.\n",
		GenerateMessageType(messageType), messageSeverities[messageSeverity],
		pCallbackData->pMessageIdName, pCallbackData->pMessage
	);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugLayerManager::DebugCallbackErrorTxt(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData
) {
	std::ofstream log("ErrorLog.txt", std::ios_base::app | std::ios_base::out);
	log << FormatDebugMessage(messageSeverity, messageType, pCallbackData);

	return VK_FALSE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugLayerManager::DebugCallbackStdError(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData
) {
	std::cerr << FormatDebugMessage(messageSeverity, messageType, pCallbackData);

	return VK_FALSE;
}

void DebugLayerManager::CreateDebugCallbacks(VkInstance instance)
{
	m_instance      = instance;

	using DebugUtil = VkInstanceExtension::VkExtDebugUtils;

	const auto callbackTypeCount = static_cast<size_t>(DebugCallbackType::None);

	for (size_t index = 0u; index < callbackTypeCount; ++index)
	{
		if (m_callbackTypes.test(index))
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = GetDebugCallbackMessengerCreateInfo(
				static_cast<DebugCallbackType>(index)
			);

			VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

			DebugUtil::vkCreateDebugUtilsMessengerEXT(
				m_instance, &createInfo, nullptr, &debugMessenger
			);

			m_debugMessengers.emplace_back(debugMessenger);
		}
	}
}

DebugLayerManager& DebugLayerManager::AddDebugCallback(DebugCallbackType type) noexcept
{
	m_callbackTypes.set(static_cast<size_t>(type));

	return *this;
}

DebugLayerManager& DebugLayerManager::AddValidationLayer(ValidationLayer layer) noexcept
{
	m_layers.emplace_back(validationLayersNames.at(static_cast<size_t>(layer)));

	return *this;
}

std::string VKAPI_CALL DebugLayerManager::GenerateMessageType(std::uint32_t typeFlag) noexcept
{
	std::string messageTypeDescription{};

	const auto messageTypeSize             = static_cast<std::uint32_t>(std::size(messageTypes));
	// Size -1 because the last entry is the invalid entry.
	const auto messageTypeCount            = messageTypeSize - 1u;
	// This would be 1 more than the value if all of bits were on for the max valid flag.
	const std::uint32_t maxMessageBitValue = 1u << messageTypeSize;

	for (std::uint32_t index = 0u; index < messageTypeCount; ++index)
	{
		const std::uint32_t flagIndex = 1u << index;

		// The second check should make sure that all of the types don't get
		// appended in case the flag was invalid, in which case it would return
		// a uint32max.
		if ((typeFlag & flagIndex) && typeFlag < maxMessageBitValue)
			messageTypeDescription
			.append(messageTypes[static_cast<size_t>(flagIndex)])
			.append(" ");
	}

	return messageTypeDescription;
}

VkDebugUtilsMessengerCreateInfoEXT DebugLayerManager::GetDebugCallbackMessengerCreateInfo(
	DebugCallbackType type
) const noexcept {
	VkDebugUtilsMessengerCreateInfoEXT createInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		// This could be some user defined data. So, it can be used to send in an
		// object to a custom logger.
		.pUserData = nullptr
	};

	if (type == DebugCallbackType::FileOut)
		createInfo.pfnUserCallback = DebugLayerManager::DebugCallbackErrorTxt;
	else
		createInfo.pfnUserCallback = DebugLayerManager::DebugCallbackStdError;

	return createInfo;
}

std::optional<std::string_view> DebugLayerManager::CheckLayerSupport() const noexcept
{
	std::uint32_t layerCount = 0u;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers{ layerCount };
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
			return requiredLayer;
	}

	return {};
}
}
