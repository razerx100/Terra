#include <cassert>
#include <ranges>
#include <algorithm>
#include <VkInstanceManager.hpp>
#include <DebugLayerManager.hpp>
#include <Exception.hpp>

VkInstanceManager::VkInstanceManager(Args& arguments)
	: m_vkInstance{ VK_NULL_HANDLE }, m_appName{ std::move(arguments.appName.value()) } {}

VkInstanceManager::~VkInstanceManager() noexcept {
	vkDestroyInstance(m_vkInstance, nullptr);
}

VkInstance VkInstanceManager::GetVKInstance() const noexcept {
	return m_vkInstance;
}

void VkInstanceManager::CheckExtensionSupport() const {
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
				"The extension "s + requiredExtension + " isn't supported."
			);
	}
}

void VkInstanceManager::CheckLayerSupport() const {
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

void VkInstanceManager::AddExtensionNames(
	const std::vector<const char*>& extensionNames
) noexcept {
	std::ranges::copy(extensionNames, std::back_inserter(m_extensionNames));
}

void VkInstanceManager::CreateInstance() {
	VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = m_appName.c_str(),
		.applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),
		.pEngineName = "Terra",
		.engineVersion = VK_MAKE_VERSION(1u, 0u, 0u),
		.apiVersion = VK_API_VERSION_1_3
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
