#include <GraphicsEngineVK.hpp>
#include <IInstanceManager.hpp>
#include <ISurfaceManager.hpp>
#include <IDeviceManager.hpp>
#include <IGraphicsQueueManager.hpp>
#include <ISwapChainManager.hpp>
#include <GraphicsPipeline.hpp>

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

	InitGraphicsQueueManagerInstance(deviceManagerRef->GetQueue(VK_QUEUE_GRAPHICS_BIT));

	InitSwapchainManagerInstance(
		deviceManagerRef->GetLogicalDevice(),
		deviceManagerRef->GetSwapChainInfo(),
		GetSurfaceManagerInstance()->GetSurface(),
		width, height
	);

	m_pipeline = std::make_unique<GraphicsPipeline>(
		deviceManagerRef->GetLogicalDevice()
		);
	m_pipeline->CreatePipelineLayout();
	m_pipeline->CreateRenderPass();
	m_pipeline->CreateGraphicsPipeline();
}

GraphicsEngineVK::~GraphicsEngineVK() noexcept {
	m_pipeline.release();
	CleanUpSwapchainManagerInstance();
	CleanUpGraphicsQueueManagerInstance();
	CleanUpDeviceManagerInstance();
	CleanUpSurfaceManagerInstance();
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
