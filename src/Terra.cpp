#ifdef TERRA_WIN32
#include <DisplayManagerWin32.hpp>
#include <SurfaceManagerWin32.hpp>
#include <VertexManagerVertex.hpp>
#else
#include <DisplayManagerVK.hpp>
#endif

#include <Terra.hpp>
#include <RenderEngineIndirectDraw.hpp>

namespace Terra {
	ObjectManager objectManager;
	std::shared_ptr<IThreadPool> threadPool;
	std::unique_ptr<DebugLayerManager> debugLayer;
	std::unique_ptr<VKCommandBuffer> graphicsCmdBuffer;
	std::unique_ptr<VKCommandBuffer> copyCmdBuffer;
	std::unique_ptr<VKCommandBuffer> computeCmdBuffer;
	std::unique_ptr<VkSyncObjects> graphicsSyncObjects;
	std::unique_ptr<VkSyncObjects> copySyncObjects;
	std::unique_ptr<VkSyncObjects> computeSyncObjects;
	std::unique_ptr<VkCommandQueue> graphicsQueue;
	std::unique_ptr<VkCommandQueue> copyQueue;
	std::unique_ptr<VkCommandQueue> computeQueue;
	std::unique_ptr<DeviceManager> device;
	std::unique_ptr<InstanceManager> vkInstance;
	std::unique_ptr<SwapChainManager> swapChain;
	std::unique_ptr<IDisplayManager> display;
	std::unique_ptr<ISurfaceManager> surface;
	std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	std::unique_ptr<RenderPassManager> renderPass;
	std::unique_ptr<BufferManager> bufferManager;
	std::unique_ptr<DescriptorSetManager> graphicsDescriptorSet;
	std::unique_ptr<DescriptorSetManager> computeDescriptorSet;
	std::unique_ptr<TextureStorage> textureStorage;
	std::unique_ptr<CameraManager> cameraManager;
	std::unique_ptr<DepthBuffer> depthBuffer;
	std::shared_ptr<ISharedDataContainer> sharedData;
	std::unique_ptr<RenderEngine> renderEngine;
	std::unique_ptr<VertexManager> vertexManager;

	namespace Resources {
		std::unique_ptr<DeviceMemory> gpuOnlyMemory;
		std::unique_ptr<DeviceMemory> uploadMemory;
		std::unique_ptr<DeviceMemory> cpuWriteMemory;
		std::unique_ptr<UploadContainer> uploadContainer;
	}

	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg) noexcept {
		objectManager.CreateObject(threadPool, std::move(threadPoolArg), 0u);
	}

	void InitGraphicsQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex,
		std::uint32_t bufferCount
	) {
		objectManager.CreateObject(graphicsQueue, { queue }, 1u);
		objectManager.CreateObject(
			graphicsCmdBuffer, { logicalDevice, queueIndex, bufferCount }, 1u
		);
		objectManager.CreateObject(
			graphicsSyncObjects, { logicalDevice, bufferCount, true }, 1u
		);
	}

	void InitCopyQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex
	) {
		objectManager.CreateObject(copyQueue, { queue }, 1u);
		objectManager.CreateObject(
			copyCmdBuffer, { .device = logicalDevice, .queueIndex = queueIndex }, 1u
		);
		objectManager.CreateObject(copySyncObjects, { .device = logicalDevice }, 1u);
	}

	void InitComputeQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex,
		std::uint32_t bufferCount
	) {
		objectManager.CreateObject(computeQueue, { queue }, 1u);
		objectManager.CreateObject(
			computeCmdBuffer, { logicalDevice, queueIndex, bufferCount }, 1u
		);
		objectManager.CreateObject(
			computeSyncObjects, { logicalDevice, bufferCount, true }, 1u
		);
	}

	void InitDisplay() {
#ifdef TERRA_WIN32
		objectManager.CreateObject<DisplayManagerWin32>(display, 3u);
#else
		objectManager.CreateObject<DisplayManagerVK>(display, 3u);
#endif
	}

	void InitSurface(VkInstance instance, void* windowHandle, void* moduleHandle) {
#ifdef TERRA_WIN32
		objectManager.CreateObject<SurfaceManagerWin32>(
			surface, { instance, windowHandle, moduleHandle }, 3u
		);
#endif
	}

	void InitDescriptorSets(VkDevice logicalDevice, std::uint32_t bufferCount) {
		objectManager.CreateObject(graphicsDescriptorSet, { logicalDevice, bufferCount }, 1u);
		objectManager.CreateObject(computeDescriptorSet, { logicalDevice, bufferCount }, 1u);
	}

	void InitRenderEngine() {
		objectManager.CreateObject<RenderEngineIndirectDraw>(renderEngine, 1u);
	}

	void InitVertexManager(VkDevice logicalDevice) {
		objectManager.CreateObject<VertexManagerVertex>(vertexManager, { logicalDevice } , 1u);
	}

	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg) noexcept {
		objectManager.CreateObject(sharedData, std::move(sharedDataArg), 0u);
	}

	void InitResources(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {
		objectManager.CreateObject(
			Resources::cpuWriteMemory,
			{ logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
			2u
		);
		objectManager.CreateObject(
			Resources::uploadMemory,
			{ logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
			2u
		);
		objectManager.CreateObject(
			Resources::gpuOnlyMemory,
			{ logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
			2u
		);

		objectManager.CreateObject(Resources::uploadContainer, 0u);
	}
}
