#include <cassert>
#include <ranges>
#include <algorithm>
#include <VkInstanceManager.hpp>
#include <Exception.hpp>
#include <format>

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

std::optional<std::string_view> VkInstanceManager::CheckExtensionSupport() const noexcept
{
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
			return requiredExtension;
	}

	return {};
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
	if (auto notSupportedLayer = m_debugLayer.CheckLayerSupport(); notSupportedLayer)
		throw Exception("Vulkan DebugLayer Error",
			std::format("The debug layer {} isn't supported.", *notSupportedLayer)
		); // Maybe I should replace this with a warning if I ever a decent logging system.

	m_extensionManager.AddExtensions(DebugLayerManager::GetRequiredExtensions());

	// Since a debugCallback can only be created after an instance has been created,
	// if someone wants to have a callback for the creation of the instance, they need
	// to pass in a VkDebugUtilsMessengerCreateInfoEXT in the pNext.

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo
		= m_debugLayer.GetDebugCallbackMessengerCreateInfo(DebugCallbackType::standardError);

	const std::vector<const char*>& validationLayers = m_debugLayer.GetActiveLayerNames();

	createInfo.enabledLayerCount   = static_cast<std::uint32_t>(std::size(validationLayers));
	createInfo.ppEnabledLayerNames = std::data(validationLayers);
	createInfo.pNext               = &debugCreateInfo;
#endif

	const std::vector<const char*>& extensionNames = m_extensionManager.GetExtensionNames();

	if (auto notSupportedExtension = CheckExtensionSupport(); notSupportedExtension)
		throw Exception("Vulkan Extension Error",
			std::format("The Instance Extension {} isn't supported.", *notSupportedExtension)
		);

	createInfo.enabledExtensionCount   = static_cast<std::uint32_t>(std::size(extensionNames));
	createInfo.ppEnabledExtensionNames = std::data(extensionNames);

	vkCreateInstance(&createInfo, nullptr, &m_vkInstance);

	m_extensionManager.PopulateExtensionFunctions(m_vkInstance);

#ifdef _DEBUG
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
