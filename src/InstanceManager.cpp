#include <InstanceManager.hpp>

void DebugLayerInst::Init(VkInstance instanceRef) {
	Set(
		CreateDebugLayerInstance(instanceRef)
	);
}

void GfxPoolInst::Init(
	VkDevice device, size_t queueIndex, size_t bufferCount
) {
	Set(
		CreateCommandPoolInstance(device, queueIndex, bufferCount)
	);
}

void DeviceInst::Init() {
	Set(
		CreateDeviceManagerInstance()
	);
}

void VkInstInst::Init(const char* appName) {
	Set(
		CreateInstanceManagerInstance(appName)
	);
}

void GfxQueInst::Init(VkQueue queue) {
	Set(
		CreateGraphicsQueueManagerInstance(queue)
	);
}

void SwapChainInst::Init(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
	VkQueue presentQueue, std::uint32_t queueFamily
) {
	Set(
		CreateSwapchainManagerInstance(
			device, swapCapabilities, surface,
			width, height, bufferCount,
			presentQueue, queueFamily
		)
	);
}

void SyncObjInst::Init(
	VkDevice device, size_t bufferCount
) {
	Set(
		CreateSyncObjectsInstance(device, bufferCount)
	);
}

void DisplayInst::Init() {
	Set(
		CreateDisplayManagerInstance()
	);
}

#ifdef TERRA_WIN32
void SurfaceInst::InitWin32(
	VkInstance instance, void* windowHandle, void* moduleHandle
) {
	Set(
		CreateWin32SurfaceManagerInstance(instance, windowHandle, moduleHandle)
	);
}
#endif
