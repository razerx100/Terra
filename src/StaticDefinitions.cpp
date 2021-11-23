#include <GraphicsEngineVK.hpp>
#include <InstanceManager.hpp>
#include <DebugLayerManager.hpp>
#include <DeviceManager.hpp>
#include <GraphicsQueueManager.hpp>
#include <ISurfaceManager.hpp>
#include <SwapChainManager.hpp>
#include <CommandPoolManager.hpp>
#include <SyncObjects.hpp>

static GraphicsEngine* s_pGraphicsEngine = nullptr;

static IInstanceManager* s_pInstanceManager = nullptr;

static DebugLayerManager* s_pDebugLayerManager = nullptr;

static IDeviceManager* s_pDeviceManager = nullptr;

static IGraphicsQueueManager* s_pGraphicsQueueManager = nullptr;

static ISurfaceManager* s_pSurfaceManager = nullptr;

static ISwapChainManager* s_pSwapChainManager = nullptr;

static ICommandPoolManager* s_pGraphicsPoolManager = nullptr;

static ISyncObjects* s_pSyncObjects = nullptr;

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
	if (s_pGraphicsEngine) {
		delete s_pGraphicsEngine;
		s_pGraphicsEngine = nullptr;
	}
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
	if (s_pInstanceManager) {
		delete s_pInstanceManager;
		s_pInstanceManager = nullptr;
	}
}

// Debug Layer
void InitDebugLayer(VkInstance instanceRef) {
	if (!s_pDebugLayerManager)
		s_pDebugLayerManager = new DebugLayerManager(instanceRef);
}

void CleanUpDebugLayer() noexcept {
	if (s_pDebugLayerManager) {
		delete s_pDebugLayerManager;
		s_pDebugLayerManager = nullptr;
	}
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
	if (s_pDeviceManager) {
		delete s_pDeviceManager;
		s_pDeviceManager = nullptr;
	}
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
	if (s_pGraphicsQueueManager) {
		delete s_pGraphicsQueueManager;
		s_pGraphicsQueueManager = nullptr;
	}
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
	if (s_pSurfaceManager) {
		delete s_pSurfaceManager;
		s_pSurfaceManager = nullptr;
	}
}

// Swapchain Manager
ISwapChainManager* GetSwapchainManagerInstance() noexcept {
	return s_pSwapChainManager;
}

void InitSwapchainManagerInstance(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	VkQueue presentQueue, std::uint32_t queueFamily
) {
	if (!s_pSwapChainManager)
		s_pSwapChainManager = new SwapChainManager(
			device, swapCapabilities, surface,
			width, height, bufferCount, presentQueue, queueFamily
		);
}

void CleanUpSwapchainManagerInstance() noexcept {
	if (s_pSwapChainManager) {
		delete s_pSwapChainManager;
		s_pSwapChainManager = nullptr;
	}
}

// Graphics Pool Manager
ICommandPoolManager* GetGraphicsPoolManagerInstance() noexcept {
	return s_pGraphicsPoolManager;
}

void InitGraphicsPoolManagerInstance(
	VkDevice device, std::uint32_t queueIndex, std::uint32_t bufferCount
) {
	if (!s_pGraphicsPoolManager)
		s_pGraphicsPoolManager = new CommandPoolManager(
			device, queueIndex, bufferCount
		);
}

void CleanUpGraphicsPoolManagerInstance() noexcept {
	if (s_pGraphicsPoolManager) {
		delete s_pGraphicsPoolManager;
		s_pGraphicsPoolManager = nullptr;
	}
}

// Sync Objects
ISyncObjects* GetSyncObjectsInstance() noexcept {
	return s_pSyncObjects;
}

void InitSyncObjectsInstance(VkDevice device, std::uint32_t bufferCount) {
	if (!s_pSyncObjects)
		s_pSyncObjects = new SyncObjects(device, bufferCount);
}

void CleanUpSyncObjectsInstance() noexcept {
	if (s_pSyncObjects) {
		delete s_pSyncObjects;
		s_pSyncObjects = nullptr;
	}
}
