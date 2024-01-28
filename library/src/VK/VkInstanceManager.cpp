#include <cassert>
#include <ranges>
#include <algorithm>
#include <VkInstanceManager.hpp>
#include <DebugLayerManager.hpp>
#include <Exception.hpp>

VkInstanceManager::VkInstanceManager(std::string_view appName)
	: m_vkInstance{ VK_NULL_HANDLE }, m_appName{ std::move(appName) },
	m_coreVersion{ CoreVersion::V1_0 }, m_extensionManager{} {}

VkInstanceManager::~VkInstanceManager() noexcept {
	vkDestroyInstance(m_vkInstance, nullptr);
}

VkInstance VkInstanceManager::GetVKInstance() const noexcept {
	return m_vkInstance;
}

void VkInstanceManager::CheckExtensionSupport() const {
	using namespace std::string_literals;

	std::uint32_t extensionCount = 0u;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(
		nullptr, &extensionCount, std::data(extensions)
	);

	for (const char* requiredExtension : m_extensionManager.GetExtensionNames())
	{
		bool found = false;
		for (const VkExtensionProperties& extension : extensions)
			if (std::strcmp(requiredExtension, extension.extensionName) == 0) {
				found = true;
				break;
			}

		if (!found)
			throw Exception("Vulkan Extension Error",
				"The extension "s + requiredExtension + " isn't supported."
			);
	}
}

void VkInstanceManager::CheckLayerSupport() const {
	using namespace std::string_literals;

	std::uint32_t layerCount = 0u;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(
		&layerCount, std::data(availableLayers)
	);

	for (const char* requiredLayer : m_validationLayersNames) {
		bool found = false;
		for (const VkLayerProperties& layer : availableLayers)
			if (std::strcmp(requiredLayer, layer.layerName) == 0) {
				found = true;
				break;
			}

		assert(found && ("The layer "s + requiredLayer + " isn't supported.").c_str());
	}
}

void VkInstanceManager::CreateInstance(CoreVersion version)
{
	m_coreVersion = version;

	VkApplicationInfo appInfo{
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName   = std::data(m_appName),
		.applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),
		.pEngineName        = "Terra",
		.engineVersion      = VK_MAKE_VERSION(1u, 0u, 0u),
		.apiVersion         = GetCoreVersion(version)
	};

	VkInstanceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo
	};

#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	if (enableValidationLayers)
	{
		CheckLayerSupport();
		createInfo.enabledLayerCount = static_cast<std::uint32_t>(
			std::size(m_validationLayersNames)
			);
		createInfo.ppEnabledLayerNames = std::data(m_validationLayersNames);

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext =
			reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
	}
	else {
		createInfo.enabledLayerCount = 0u;
		createInfo.pNext = nullptr;
	}

	const std::vector<const char*>& extensionNames = m_extensionManager.GetExtensionNames();

	CheckExtensionSupport();
	createInfo.enabledExtensionCount   = static_cast<std::uint32_t>(std::size(extensionNames));
	createInfo.ppEnabledExtensionNames = std::data(extensionNames);

	vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
}

std::uint32_t VkInstanceManager::GetCoreVersion(CoreVersion version) noexcept
{
	std::uint32_t coreVersion = 0u;

	if (version == CoreVersion::V1_0)
		coreVersion = VK_API_VERSION_1_0;
	else if (version == CoreVersion::V1_1)
		coreVersion = VK_API_VERSION_1_1;
	else if (version == CoreVersion::V1_2)
		coreVersion = VK_API_VERSION_1_2;
	else if (version == CoreVersion::V1_3)
		coreVersion = VK_API_VERSION_1_3;

	return coreVersion;
}
