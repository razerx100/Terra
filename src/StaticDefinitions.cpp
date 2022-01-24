#include <VkInstanceManager.hpp>
#include <DebugLayerManager.hpp>
#include <DeviceManager.hpp>
#include <GraphicsQueueManager.hpp>
#include <ISurfaceManager.hpp>
#include <SwapChainManager.hpp>
#include <CommandPoolManager.hpp>
#include <IDisplayManager.hpp>
#include <CopyQueueManager.hpp>
#include <ResourceBuffer.hpp>
#include <FenceWrapper.hpp>
#include <SemaphoreWrapper.hpp>
#include <ViewportAndScissorManager.hpp>

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
IGraphicsQueueManager* CreateGraphicsQueueManagerInstance(
	VkDevice device, VkQueue queue, size_t bufferCount
) {
	return new GraphicsQueueManager(device, queue, bufferCount);
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
	std::uint32_t width, std::uint32_t height, size_t bufferCount,
	VkQueue presentQueue, size_t queueFamily
) {
	return new SwapChainManager(
		device, swapCapabilities, surface,
		width, height, bufferCount, presentQueue, queueFamily
	);
}

// Graphics Pool Manager
ICommandPoolManager* CreateCommandPoolInstance(
	VkDevice device, size_t queueIndex, size_t bufferCount
) {
	return new CommandPoolManager(
		device, queueIndex, bufferCount
	);
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

// Copy Queue
ICopyQueueManager* CreateCopyQueueManagerInstance(
	VkDevice device,
	VkQueue queue
) {
	return new CopyQueueManager(device, queue);
}

// Resource Buffer
IResourceBuffer* CreateResourceBufferInstance(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	BufferType type
) {
	return new ResourceBuffer(logDevice, phyDevice, queueFamilyIndices,type);
}

// Fence Wrapper
IFenceWrapper* CreateFenceWrapperInstance(
	VkDevice device, size_t bufferCount, bool signaled
) {
	return new FenceWrapper(device, bufferCount, signaled);
}

// Semaphore Wrapper
ISemaphoreWrapper* CreateSemaphoreWrapperInstance(
	VkDevice device, size_t bufferCount
) {
	return new SemaphoreWrapper(device, bufferCount);
}

// Viewport and Scissor
IViewportAndScissorManager* CreateViewportAndScissorInstance(
	std::uint32_t width, std::uint32_t height
) {
	return new ViewportAndScissorManager(width, height);
}
