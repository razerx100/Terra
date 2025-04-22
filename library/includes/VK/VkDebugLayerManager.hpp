#ifndef VK_DEBUG_LAYER_MANAGER_HPP_
#define VK_DEBUG_LAYER_MANAGER_HPP_
#include <vulkan/vulkan.h>
#include <string>
#include <VkExtensionManager.hpp>
#include <array>
#include <vector>
#include <optional>
#include <bitset>

enum class ValidationLayer
{
	VkLayerKhronosValidation
};

enum class DebugCallbackType
{
	StandardError,
	FileOut,
	None
};

class DebugLayerManager
{
public:
	DebugLayerManager();

	DebugLayerManager& AddValidationLayer(ValidationLayer layer) noexcept;

	DebugLayerManager& AddDebugCallback(DebugCallbackType type) noexcept;

	void CreateDebugCallbacks(VkInstance instance);
	void DestroyDebugCallbacks() noexcept;

	[[nodiscard]]
	VkDebugUtilsMessengerCreateInfoEXT GetDebugCallbackMessengerCreateInfo(
		DebugCallbackType type
	) const noexcept;
	[[nodiscard]]
	const std::vector<const char*>& GetActiveLayerNames() const noexcept { return m_layers; }
	[[nodiscard]]
	// Returns empty if all of the layers are supported. If a layer isn't supported
	// returns its name.
	std::optional<std::string_view> CheckLayerSupport() const noexcept;

private:
	std::vector<VkDebugUtilsMessengerEXT>                     m_debugMessengers;
	std::bitset<static_cast<size_t>(DebugCallbackType::None)> m_callbackTypes;
	VkInstance                                                m_instance;
	std::vector<const char*>                                  m_layers;

public:
	DebugLayerManager(const DebugLayerManager&) = delete;
	DebugLayerManager& operator=(const DebugLayerManager&) = delete;

	DebugLayerManager(DebugLayerManager&& other) noexcept
		: m_debugMessengers{ std::move(other.m_debugMessengers) },
		m_callbackTypes{ std::move(other.m_callbackTypes) },
		m_instance{ other.m_instance }, m_layers{ std::move(other.m_layers) } {}

	DebugLayerManager& operator=(DebugLayerManager&& other) noexcept
	{
		m_debugMessengers = std::move(other.m_debugMessengers);
		m_callbackTypes   = std::move(other.m_callbackTypes);
		m_instance        = other.m_instance;
		m_layers          = std::move(other.m_layers);

		return *this;
	}

private:
	static constexpr std::array s_requiredExtensions
	{
		InstanceExtension::VkExtDebugUtils
	};

public:
	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{
		return s_requiredExtensions;
	}

private:
	[[nodiscard]]
	static std::string VKAPI_CALL GenerateMessageType(std::uint32_t typeFlag) noexcept;
	[[nodiscard]]
	static std::string VKAPI_CALL FormatDebugMessage(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
	);

public:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackErrorTxt(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallbackStdError(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);
};
#endif
