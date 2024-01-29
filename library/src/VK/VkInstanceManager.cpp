#include <cassert>
#include <ranges>
#include <algorithm>
#include <VkInstanceManager.hpp>
#include <Exception.hpp>

VkInstanceManager::VkInstanceManager(std::string_view appName)
	: m_vkInstance{ VK_NULL_HANDLE }, m_appName{ std::move(appName) },
	m_coreVersion{ CoreVersion::V1_0 }, m_extensionManager{}, m_debugLayer{} {}

VkInstanceManager::~VkInstanceManager() noexcept
{
#ifdef _DEBUG
	m_debugLayer.DestroyDebugCallbacks();
#endif
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
		.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo
	};

#ifdef _DEBUG
	m_extensionManager.AddExtensions(DebugLayerManager::GetRequiredExtensions());

	const bool allValidationLayerSupported = m_debugLayer.CheckLayerSupport();

	// I suppose you need put one of these struct in the pNext chain for each callback.
	// Need to fix that later.
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	if (allValidationLayerSupported)
	{
		debugCreateInfo = m_debugLayer.GetDebugCallbackMessengerCreateInfo();

		const std::vector<const char*>& validationLayers   = m_debugLayer.GetActiveLayerNames();

		createInfo.enabledLayerCount                       = static_cast<std::uint32_t>(std::size(validationLayers));
		createInfo.ppEnabledLayerNames                     = std::data(validationLayers);
		createInfo.pNext                                   = &debugCreateInfo; // This line is required
		// for callbacks only. Not putting a callback function will cause the instance to be not
		// created.
	}
	// else log that all of the validation layers aren't supported. Maybe even mention which one.
#endif

	const std::vector<const char*>& extensionNames = m_extensionManager.GetExtensionNames();

	CheckExtensionSupport();
	createInfo.enabledExtensionCount   = static_cast<std::uint32_t>(std::size(extensionNames));
	createInfo.ppEnabledExtensionNames = std::data(extensionNames);

	vkCreateInstance(&createInfo, nullptr, &m_vkInstance);

	m_extensionManager.PopulateExtensionFunctions(m_vkInstance);

#ifdef _DEBUG
	if (allValidationLayerSupported)
		m_debugLayer.CreateDebugCallbacks(m_vkInstance);
#endif
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
