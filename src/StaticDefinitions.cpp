#include <VkInstanceManager.hpp>
#include <DebugLayerManager.hpp>
#include <DeviceManager.hpp>
#include <GraphicsQueueManager.hpp>
#include <ISurfaceManager.hpp>
#include <SwapChainManager.hpp>
#include <CommandPoolManager.hpp>
#include <SyncObjects.hpp>
#include <IDisplayManager.hpp>

// Instance Manager
IInstanceManager* CreateInstanceManagerInstance(const char* appName) {
	return new InstanceManager(appName);
}

// Debug Layer
DebugLayerManager* CreateDebugLayerInstance(VkInstance instanceRef) {
	return new DebugLayerManager(instanceRef);
}

// Device Manager
IDeviceManager* CreateDeviceManagerInstance() {
	return new DeviceManager();
}

// Graphics Queue Manager
IGraphicsQueueManager* CreateGraphicsQueueManagerInstance(VkQueue queue) {
	return new GraphicsQueueManager(queue);
}

// Surface Manager
#ifdef TERRA_WIN32
#include <SurfaceManagerWin32.hpp>

ISurfaceManager* CreateWin32SurfaceManagerInstance(
	VkInstance instance, void* windowHandle, void* moduleHandle
) {
	return new SurfaceManagerWin32(instance, windowHandle, moduleHandle);
}

#endif

// Swapchain Manager
ISwapChainManager* CreateSwapchainManagerInstance(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	VkQueue presentQueue, std::uint32_t queueFamily
) {
	return new SwapChainManager(
		device, swapCapabilities, surface,
		width, height, bufferCount, presentQueue, queueFamily
	);
}

// Graphics Pool Manager
ICommandPoolManager* CreateCommandPoolInstance(
	VkDevice device, std::uint32_t queueIndex, std::uint32_t bufferCount
) {
	return new CommandPoolManager(
		device, queueIndex, bufferCount
	);
}

// Sync Objects
ISyncObjects* CreateSyncObjectsInstance(VkDevice device, std::uint32_t bufferCount) {
	return new SyncObjects(device, bufferCount);
}

// Display Manager
#ifdef TERRA_WIN32
#include <DisplayManagerWin32.hpp>
#else
#include <DisplayManagerVK.hpp>
#endif

IDisplayManager* CreateDisplayManagerInstance() {
#ifdef TERRA_WIN32
	return new DisplayManagerWin32();
#else
	return new DisplayManagerVK();
#endif
}
