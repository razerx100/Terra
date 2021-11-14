#include <GraphicsEngineVK.hpp>
#include <InstanceManager.hpp>
#include <DebugLayerManager.hpp>
#include <DeviceManager.hpp>
#include <GraphicsQueueManager.hpp>
#include <ISurfaceManager.hpp>
#include <SwapChainManager.hpp>

static GraphicsEngine* s_pGraphicsEngine = nullptr;

static IInstanceManager* s_pInstanceManager = nullptr;

static DebugLayerManager* s_pDebugLayerManager = nullptr;

static IDeviceManager* s_pDeviceManager = nullptr;

static IGraphicsQueueManager* s_pGraphicsQueueManager = nullptr;

static ISurfaceManager* s_pSurfaceManager = nullptr;

static ISwapChainManager* s_pSwapChainManager = nullptr;

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
IInstanceManager* GetInstanceManagerInstance() noexcept {
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
IDeviceManager* GetDeviceManagerInstance() noexcept {
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

// Graphics Queue Manager
IGraphicsQueueManager* GetGraphicsQueueManagerInstance() noexcept {
	return s_pGraphicsQueueManager;
}

void InitGraphicsQueueManagerInstance(VkQueue queue) {
	if (!s_pGraphicsQueueManager)
		s_pGraphicsQueueManager = new GraphicsQueueManager(queue);
}

void CleanUpGraphicsQueueManagerInstance() noexcept {
	if (s_pGraphicsQueueManager)
		delete s_pGraphicsQueueManager;
}

// Surface Manager
#ifdef TERRA_WIN32
#include <Win32/SurfaceManagerWin32.hpp>
#endif

ISurfaceManager* GetSurfaceManagerInstance() noexcept {
	return s_pSurfaceManager;
}

void InitSurfaceManagerInstance(VkInstance instance, void* windowHandle, void* moduleHandle) {
	if (!s_pSurfaceManager)
#ifdef TERRA_WIN32
		s_pSurfaceManager = new SurfaceManagerWin32(instance, windowHandle, moduleHandle);
#endif
}

void CleanUpSurfaceManagerInstance() noexcept {
	if (s_pSurfaceManager)
		delete s_pSurfaceManager;
}

// Swapchain Manager
ISwapChainManager* GetSwapchainManagerInstance() noexcept {
	return s_pSwapChainManager;
}

void InitSwapchainManagerInstance(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height
) {
	if (!s_pSwapChainManager)
		s_pSwapChainManager = new SwapChainManager(
			device, swapCapabilities, surface,
			width, height
		);
}

void CleanUpSwapchainManagerInstance() noexcept {
	if (s_pSwapChainManager)
		delete s_pSwapChainManager;
}
