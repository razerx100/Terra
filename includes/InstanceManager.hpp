#ifndef __INSTANCE_MANAGER_HPP__
#define __INSTANCE_MANAGER_HPP__
#include <ObjectManager.hpp>
#include <DebugLayerManager.hpp>
#include <ICommandPoolManager.hpp>
#include <IDeviceManager.hpp>
#include <IVkInstanceManager.hpp>
#include <IGraphicsQueueManager.hpp>
#include <ISurfaceManager.hpp>
#include <ISwapChainManager.hpp>
#include <ISyncObjects.hpp>
#include <IDisplayManager.hpp>

class DebugLayerInst : public _ObjectManager<DebugLayerManager, DebugLayerInst> {
public:
	static void Init(VkInstance instanceRef);
};

class GfxPoolInst : public _ObjectManager<ICommandPoolManager, GfxPoolInst> {
public:
	static void Init(
		VkDevice device, size_t queueIndex, size_t bufferCount
	);
};

class DeviceInst : public _ObjectManager<IDeviceManager, DeviceInst> {
public:
	static void Init();
};

class VkInstInst : public _ObjectManager<IInstanceManager, VkInstInst> {
public:
	static void Init(const char* appName);
};

class GfxQueInst : public _ObjectManager<IGraphicsQueueManager, GfxQueInst> {
public:
	static void Init(VkQueue queue);
};

class SwapChainInst : public _ObjectManager<ISwapChainManager, SwapChainInst> {
public:
	static void Init(
		VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
		std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		VkQueue presentQueue, std::uint32_t queueFamily
	);
};

class SyncObjInst : public _ObjectManager<ISyncObjects, SyncObjInst> {
public:
	static void Init(
		VkDevice device, size_t bufferCount
	);
};

class DisplayInst : public _ObjectManager<IDisplayManager, DisplayInst> {
public:
	static void Init();
};

class SurfaceInst : public _ObjectManager<ISurfaceManager, SurfaceInst> {
public:
#ifdef TERRA_WIN32
	static void InitWin32(
		VkInstance instance, void* windowHandle, void* moduleHandle
	);
#endif
};

#endif
