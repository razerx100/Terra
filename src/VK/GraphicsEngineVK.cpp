#include <GraphicsEngineVK.hpp>

GraphicsEngineVK::GraphicsEngineVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) : m_backgroundColor{ 0.1f, 0.1f, 0.1f, 0.1f }, m_appName(appName) {
	CreateInstance();

}

GraphicsEngineVK::~GraphicsEngineVK() noexcept {
	vkDestroyInstance(m_vkInstance, nullptr);
}

void GraphicsEngineVK::SetBackgroundColor(Color color) noexcept {

}

void GraphicsEngineVK::SubmitCommands() {

}

void GraphicsEngineVK::Render() {

}

void GraphicsEngineVK::Resize(std::uint32_t width, std::uint32_t height) {

}

SRect GraphicsEngineVK::GetMonitorCoordinates() {
	return SRect{};
}

void GraphicsEngineVK::CreateInstance() {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Terra";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<std::uint32_t>(
			m_extensionNames.size()
		);
	createInfo.ppEnabledLayerNames = m_extensionNames.data();
	createInfo.enabledLayerCount = 0u;

	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
}
