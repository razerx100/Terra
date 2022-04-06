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
#include <IDisplayManager.hpp>
#include <ICopyQueueManager.hpp>
#include <IResourceBuffer.hpp>
#include <IViewportAndScissorManager.hpp>
#include <IRenderPassManager.hpp>
#include <IModelContainer.hpp>
#include <IDescriptorSetManager.hpp>

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

class CpyPoolInst : public _ObjectManager<ICommandPoolManager, CpyPoolInst> {
public:
	static void Init(
		VkDevice device, size_t queueIndex
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
	static void Init(VkDevice device, VkQueue queue, size_t bufferCount);
};

class CpyQueInst : public _ObjectManager<ICopyQueueManager, CpyQueInst> {
public:
	static void Init(VkDevice device, VkQueue queue);
};

class SwapChainInst : public _ObjectManager<ISwapChainManager, SwapChainInst> {
public:
	static void Init(
		VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
		std::uint32_t width, std::uint32_t height, size_t bufferCount,
		VkQueue presentQueue, size_t queueFamily
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

class VertexBufferInst : public _ObjectManager<IResourceBuffer, VertexBufferInst> {
public:
	static void Init(
		VkDevice logDevice, VkPhysicalDevice phyDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
};

class IndexBufferInst : public _ObjectManager<IResourceBuffer, IndexBufferInst> {
public:
	static void Init(
		VkDevice logDevice, VkPhysicalDevice phyDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
};

class ViewPAndScsrInst : public _ObjectManager<IViewportAndScissorManager, ViewPAndScsrInst> {
public:
	static void Init(std::uint32_t width, std::uint32_t height);
};

class RndrPassInst : public _ObjectManager<IRenderPassManager, RndrPassInst> {
public:
	static void Init(VkDevice device, VkFormat swapchainFormat);
};

class ModelContainerInst : public _ObjectManager<IModelContainer, ModelContainerInst> {
public:
	static void Init(const char* shaderPath);
};

class UniformBufferInst : public _ObjectManager<IResourceBuffer, UniformBufferInst> {
public:
	static void Init(
		VkDevice logDevice, VkPhysicalDevice phyDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
};

class DescSetMan : public _ObjectManager<IDescriptorSetManager, DescSetMan> {
public:
	static void Init(VkDevice device);
};
#endif
