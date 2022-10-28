#include <VkInstanceManager.hpp>
#include <DebugLayerManager.hpp>
#include <cassert>
#include <Exception.hpp>

InstanceManager::InstanceManager(const char* appName) noexcept
	: m_vkInstance(VK_NULL_HANDLE), m_appName(appName) {}

InstanceManager::~InstanceManager() noexcept {
	vkDestroyInstance(m_vkInstance, nullptr);
}

VkInstance InstanceManager::GetVKInstance() const noexcept {
	return m_vkInstance;
}

void InstanceManager::CheckExtensionSupport() const {
	std::uint32_t extensionCount = 0u;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(
		nullptr, &extensionCount, std::data(extensions)
	);

	for (const char* requiredExtension : m_extensionNames) {
		bool found = false;
		for (const VkExtensionProperties& extension : extensions)
			if (std::strcmp(requiredExtension, extension.extensionName) == 0) {
				found = true;
				break;
			}

		if (!found)
			throw Exception("Vulkan Extension Error",
				std::string("The extension ") + requiredExtension + " isn't supported."
			);
	}
}

void InstanceManager::CheckLayerSupport() const {
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

		assert(
			found && (std::string("The layer ") + requiredLayer + " isn't supported.").c_str()
		);
	}
}

void InstanceManager::AddExtensionNames(
	const std::vector<const char*>& extensionNames
) noexcept {
	m_extensionNames.insert(
		std::end(m_extensionNames),
		std::begin(extensionNames), std::end(extensionNames)
	);
}

void InstanceManager::CreateInstance() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u);
	appInfo.pEngineName = "Terra";
	appInfo.engineVersion = VK_MAKE_VERSION(1u, 0u, 0u);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

	if (enableValidationLayers) {
		CheckLayerSupport();
		createInfo.enabledLayerCount = static_cast<std::uint32_t>(
			std::size(m_validationLayersNames)
			);
		createInfo.ppEnabledLayerNames = std::data(m_validationLayersNames);

		m_extensionNames.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext =
			reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
	}
	else {
		createInfo.enabledLayerCount = 0u;
		createInfo.pNext = nullptr;
	}

#ifdef TERRA_WIN32
	m_extensionNames.emplace_back("VK_KHR_win32_surface");
#endif

	CheckExtensionSupport();
	createInfo.enabledExtensionCount = static_cast<std::uint32_t>(
		std::size(m_extensionNames)
		);
	createInfo.ppEnabledExtensionNames = std::data(m_extensionNames);

	vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
}
