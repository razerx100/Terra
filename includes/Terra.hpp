#ifndef TERRA_HPP_
#define TERRA_HPP_
#include <IThreadPool.hpp>
#include <DebugLayerManager.hpp>
#include <VkCommandQueue.hpp>
#include <VkSyncObjects.hpp>
#include <DeviceManager.hpp>
#include <VKInstanceManager.hpp>
#include <ISurfaceManager.hpp>
#include <SwapChainManager.hpp>
#include <IDisplayManager.hpp>
#include <ViewportAndScissorManager.hpp>
#include <RenderPassManager.hpp>
#include <DescriptorSetManager.hpp>
#include <TextureStorage.hpp>
#include <DepthBuffer.hpp>
#include <memory>
#include <DeviceMemory.hpp>
#include <UploadContainer.hpp>
#include <VertexManager.hpp>
#include <ObjectManager.hpp>

#include <RenderEngine.hpp>
#include <BufferManager.hpp>
#include <ISharedDataContainer.hpp>
#include <CameraManager.hpp>

namespace Terra {
	// Variables
	extern ObjectManager objectManager;
	extern std::shared_ptr<IThreadPool> threadPool;
	extern std::unique_ptr<DebugLayerManager> debugLayer;
	extern std::unique_ptr<VKCommandBuffer> graphicsCmdBuffer;
	extern std::unique_ptr<VKCommandBuffer> copyCmdBuffer;
	extern std::unique_ptr<VKCommandBuffer> computeCmdBuffer;
	extern std::unique_ptr<VkCommandQueue> graphicsQueue;
	extern std::unique_ptr<VkSyncObjects> graphicsSyncObjects;
	extern std::unique_ptr<VkSyncObjects> copySyncObjects;
	extern std::unique_ptr<VkSyncObjects> computeSyncObjects;
	extern std::unique_ptr<VkCommandQueue> copyQueue;
	extern std::unique_ptr<VkCommandQueue> computeQueue;
	extern std::unique_ptr<DeviceManager> device;
	extern std::unique_ptr<InstanceManager> vkInstance;
	extern std::unique_ptr<SwapChainManager> swapChain;
	extern std::unique_ptr<IDisplayManager> display;
	extern std::unique_ptr<ISurfaceManager> surface;
	extern std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	extern std::unique_ptr<RenderPassManager> renderPass;
	extern std::unique_ptr<BufferManager> bufferManager;
	extern std::unique_ptr<DescriptorSetManager> graphicsDescriptorSet;
	extern std::unique_ptr<DescriptorSetManager> computeDescriptorSet;
	extern std::unique_ptr<TextureStorage> textureStorage;
	extern std::unique_ptr<CameraManager> cameraManager;
	extern std::unique_ptr<DepthBuffer> depthBuffer;
	extern std::shared_ptr<ISharedDataContainer> sharedData;
	extern std::unique_ptr<RenderEngine> renderEngine;
	extern std::unique_ptr<VertexManager> vertexManager;

	namespace Resources {
		extern std::unique_ptr<DeviceMemory> gpuOnlyMemory;
		extern std::unique_ptr<DeviceMemory> uploadMemory;
		extern std::unique_ptr<DeviceMemory> cpuWriteMemory;
		extern std::unique_ptr<UploadContainer> uploadContainer;
	}

	// Initialization functions
	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg) noexcept;
	void InitGraphicsQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex,
		std::uint32_t bufferCount
	);
	void InitCopyQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex
	);
	void InitComputeQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex,
		std::uint32_t bufferCount
	);
	void InitDisplay();
	void InitSurface(VkInstance instance, void* windowHandle, void* moduleHandle);
	void InitDescriptorSets(VkDevice logicalDevice, std::uint32_t bufferCount);
	void InitRenderEngine();
	void InitVertexManager(VkDevice logicalDevice);
	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg) noexcept;
	void InitResources(VkPhysicalDevice physicalDevice, VkDevice logicalDevice);
}
#endif
