#include <GraphicsEngineVK.hpp>
#include <InstanceManager.hpp>
#include <DebugLayerManager.hpp>
#include <DeviceManager.hpp>

static GraphicsEngine* s_pGraphicsEngine = nullptr;

static InstanceManager* s_pInstanceManager = nullptr;

static DebugLayerManager* s_pDebugLayerManager = nullptr;

static DeviceManager* s_pDeviceManager = nullptr;

// Graphics Engine
GraphicsEngine* GetGraphicsEngineInstance() noexcept {
	return s_pGraphicsEngine;
}

void InitGraphicsEngineInstance(
	const char* appName,
	void* windowHandle,
	void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) {
	if (!s_pGraphicsEngine)
		s_pGraphicsEngine = new GraphicsEngineVK(
			appName,
			windowHandle, moduleHandle, width, height, bufferCount
		);
}

void CleanUpGraphicsEngineInstance() noexcept {
	if (s_pGraphicsEngine)
		delete s_pGraphicsEngine;
}

// Instance Manager
InstanceManager* GetInstanceManagerInstance() noexcept {
	return s_pInstanceManager;
}

void InitInstanceManagerInstance(const char* appName) {
	if (!s_pInstanceManager)
		s_pInstanceManager = new InstanceManager(appName);
}

void CleanUpInstanceManagerInstance() noexcept {
	if (s_pInstanceManager)
		delete s_pInstanceManager;
}

// Debug Layer
void InitDebugLayer(VkInstance instanceRef) {
	if (!s_pDebugLayerManager)
		s_pDebugLayerManager = new DebugLayerManager(instanceRef);
}

void CleanUpDebugLayer() noexcept {
	if (s_pDebugLayerManager)
		delete s_pDebugLayerManager;
}

// Device Manager
DeviceManager* GetDeviceManagerInstance() noexcept {
	return s_pDeviceManager;
}

void InitDeviceManagerInstance() {
	if (!s_pDeviceManager)
		s_pDeviceManager = new DeviceManager();
}

void CleanUpDeviceManagerInstance() noexcept {
	if (s_pDeviceManager)
		delete s_pDeviceManager;
}
