#include <GraphicsEngineVK.hpp>
#include <InstanceManager.hpp>
#include <DeviceManager.hpp>

GraphicsEngineVK::GraphicsEngineVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) : m_backgroundColor{ 0.1f, 0.1f, 0.1f, 0.1f }, m_appName(appName) {
	InitInstanceManagerInstance(appName);

	InitDeviceManagerInstance();
	DeviceManager* deviceManagerRef = GetDeviceManagerInstance();
	deviceManagerRef->AddQueueTypeCheck(VK_QUEUE_GRAPHICS_BIT);
	deviceManagerRef->CreatePhysicalDevice(GetInstanceManagerInstance()->GetVKInstance());
}

GraphicsEngineVK::~GraphicsEngineVK() noexcept {
	CleanUpInstanceManagerInstance();
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
