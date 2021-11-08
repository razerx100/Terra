#include <InstanceManager.hpp>
#include <VKThrowMacros.hpp>
#include <DebugLayerManager.hpp>

InstanceManager::InstanceManager(const char* appName) {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Terra";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

#ifdef _DEBUG
	const bool enableValidationLayers = true;
	InitDebugLayer(m_vkInstance);
#else
	const bool enableValidationLayers = false;
#endif

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

	if (enableValidationLayers) {
		CheckLayerSupport();
		createInfo.enabledLayerCount = static_cast<std::uint32_t>(
			m_validationLayersNames.size()
			);
		createInfo.ppEnabledLayerNames = m_validationLayersNames.data();

		m_extensionNames.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		DebugLayerManager::PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext =
			reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
	}
	else {
		createInfo.enabledLayerCount = 0u;
		createInfo.pNext = nullptr;
	}

	CheckExtensionSupport();
	createInfo.enabledExtensionCount = static_cast<std::uint32_t>(
		m_extensionNames.size()
		);
	createInfo.ppEnabledExtensionNames = m_extensionNames.data();

	VkResult result;
	VK_THROW_FAILED(result, vkCreateInstance(&createInfo, nullptr, &m_vkInstance));
}

InstanceManager::~InstanceManager() noexcept {
#ifdef _DEBUG
	CleanUpDebugLayer();
#endif
	vkDestroyInstance(m_vkInstance, nullptr);
}

VkInstance InstanceManager::GetInstanceRef() const noexcept {
	return m_vkInstance;
}

void InstanceManager::CheckExtensionSupport() const {
	std::uint32_t extensionCount = 0u;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(
		nullptr, &extensionCount, extensions.data()
	);

	for (const char* requiredExtension : m_extensionNames) {
		bool found = false;
		for (const VkExtensionProperties& extension : extensions)
			if (std::strcmp(requiredExtension, extension.extensionName) == 0) {
				found = true;
				break;
			}

		if (!found)
			VK_GENERIC_THROW(
				(std::string("The extension ")
					+ requiredExtension + " isn't supported."
					).c_str()
			);
	}
}

void InstanceManager::CheckLayerSupport() const {
	std::uint32_t layerCount = 0u;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(
		&layerCount, availableLayers.data()
	);

	for (const char* requiredLayer : m_validationLayersNames) {
		bool found = false;
		for (const VkLayerProperties& layer : availableLayers)
			if (std::strcmp(requiredLayer, layer.layerName) == 0) {
				found = true;
				break;
			}

		if (!found)
			VK_GENERIC_THROW(
				(std::string("The layer ")
					+ requiredLayer + " isn't supported."
					).c_str()
			);
	}
}
