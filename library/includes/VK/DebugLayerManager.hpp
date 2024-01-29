#ifndef DEBUG_LAYER_MANAGER_HPP_
#define DEBUG_LAYER_MANAGER_HPP_
#include <vulkan/vulkan.h>
#include <string>
#include <VkExtensionManager.hpp>
#include <array>
#include <vector>

enum class ValidationLayer
{
	VkLayerKhronosValidation
};

enum class DebugCallbackType
{
	standardOut,
	FileOut
};

class DebugLayerManager
{
public:
	DebugLayerManager();

	void AddValidationLayer(ValidationLayer layer) noexcept;

	void AddDebugCallback(DebugCallbackType type) noexcept;

	void CreateDebugCallbacks(VkInstance instance);
	void DestroyDebugCallbacks() noexcept;

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);

	[[nodiscard]]
	VkDebugUtilsMessengerCreateInfoEXT GetDebugCallbackMessengerCreateInfo() const noexcept;
	[[nodiscard]]
	const std::vector<const char*>& GetActiveLayerNames() const noexcept
	{ return m_layers; }
	[[nodiscard]]
	bool CheckLayerSupport() const noexcept;

private:
	static std::string GenerateMessageType(std::uint32_t typeFlag) noexcept;

private:
	std::vector<VkDebugUtilsMessengerEXT> m_debugMessengers;
	std::vector<DebugCallbackType>        m_callbackTypes;
	VkInstance                            m_pInstanceRef;
	std::vector<const char*>              m_layers;

public:
	DebugLayerManager(const DebugLayerManager&) = delete;
	DebugLayerManager& operator=(const DebugLayerManager&) = delete;

	DebugLayerManager(DebugLayerManager&& other) noexcept
		: m_debugMessengers{ std::move(other.m_debugMessengers) },
		m_callbackTypes{ std::move(other.m_callbackTypes) },
		m_pInstanceRef{ other.m_pInstanceRef }, m_layers{ std::move(other.m_layers) } {}

	DebugLayerManager& operator=(DebugLayerManager&& other) noexcept
	{
		m_debugMessengers = std::move(other.m_debugMessengers);
		m_callbackTypes   = std::move(other.m_callbackTypes);
		m_pInstanceRef    = other.m_pInstanceRef;
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
	{ return s_requiredExtensions; }
};
#endif
