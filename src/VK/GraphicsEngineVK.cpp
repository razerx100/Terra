#include <GraphicsEngineVK.hpp>
#include <IInstanceManager.hpp>
#include <ISurfaceManager.hpp>
#include <IDeviceManager.hpp>
#include <IGraphicsQueueManager.hpp>
#include <ISwapChainManager.hpp>
#include <ICommandPoolManager.hpp>
#include <SyncObjects.hpp>

GraphicsEngineVK::GraphicsEngineVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) : m_backgroundColor{ 0.1f, 0.1f, 0.1f, 0.1f }, m_appName(appName) {

	InitInstanceManagerInstance(appName);

	InitSurfaceManagerInstance(
		GetInstanceManagerInstance()->GetVKInstance(), windowHandle, moduleHandle
	);

	InitDeviceManagerInstance();
	IDeviceManager* deviceManagerRef = GetDeviceManagerInstance();
	deviceManagerRef->CreatePhysicalDevice(
		GetInstanceManagerInstance()->GetVKInstance(),
		GetSurfaceManagerInstance()->GetSurface()
	);
	deviceManagerRef->CreateLogicalDevice();

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = deviceManagerRef->GetQueue(
		QueueType::GraphicsQueue
	);
	InitGraphicsQueueManagerInstance(
		graphicsQueueHandle
	);

	VkDevice logicalDevice = deviceManagerRef->GetLogicalDevice();

	InitSwapchainManagerInstance(
		logicalDevice,
		deviceManagerRef->GetSwapChainInfo(),
		GetSurfaceManagerInstance()->GetSurface(),
		width, height, bufferCount
	);

	InitGraphicsPoolManagerInstance(
		logicalDevice,
		graphicsQueueFamilyIndex,
		bufferCount
	);

	InitSyncObjectsInstance(
		logicalDevice,
		bufferCount
	);
}

GraphicsEngineVK::~GraphicsEngineVK() noexcept {
	CleanUpGraphicsPoolManagerInstance();
	CleanUpSwapchainManagerInstance();
	CleanUpGraphicsQueueManagerInstance();
	CleanUpSyncObjectsInstance();
	CleanUpDeviceManagerInstance();
	CleanUpSurfaceManagerInstance();
	CleanUpInstanceManagerInstance();
}

void GraphicsEngineVK::SetBackgroundColor(Color color) noexcept {
	m_backgroundColor = color;
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

void GraphicsEngineVK::WaitForAsyncTasks() {
	vkDeviceWaitIdle(
		GetDeviceManagerInstance()->GetLogicalDevice()
	);
}
