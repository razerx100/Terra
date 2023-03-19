#ifdef TERRA_WIN32
#include <DisplayManagerWin32.hpp>
#include <SurfaceManagerWin32.hpp>
#include <Exception.hpp>
#else
#include <DisplayManagerVK.hpp>
#endif

#include <Terra.hpp>
#include <RenderEngineVertexShader.hpp>

namespace Terra {
	std::shared_ptr<IThreadPool> threadPool;
	std::unique_ptr<DebugLayerManager> debugLayer;
	std::unique_ptr<VKCommandBuffer> graphicsCmdBuffer;
	std::unique_ptr<VKCommandBuffer> transferCmdBuffer;
	std::unique_ptr<VKCommandBuffer> computeCmdBuffer;
	std::unique_ptr<VkSyncObjects> graphicsSyncObjects;
	std::unique_ptr<VkSyncObjects> transferSyncObjects;
	std::unique_ptr<VkSyncObjects> computeSyncObjects;
	std::unique_ptr<VkCommandQueue> graphicsQueue;
	std::unique_ptr<VkCommandQueue> transferQueue;
	std::unique_ptr<VkCommandQueue> computeQueue;
	std::unique_ptr<DeviceManager> device;
	std::unique_ptr<InstanceManager> vkInstance;
	std::unique_ptr<SwapChainManager> swapChain;
	std::unique_ptr<IDisplayManager> display;
	std::unique_ptr<ISurfaceManager> surface;
	std::unique_ptr<BufferManager> bufferManager;
	std::unique_ptr<DescriptorSetManager> graphicsDescriptorSet;
	std::unique_ptr<DescriptorSetManager> computeDescriptorSet;
	std::unique_ptr<TextureStorage> textureStorage;
	std::unique_ptr<CameraManager> cameraManager;
	std::shared_ptr<ISharedDataContainer> sharedData;
	std::unique_ptr<RenderEngine> renderEngine;

	namespace Resources {
		std::unique_ptr<DeviceMemory> gpuOnlyMemory;
		std::unique_ptr<DeviceMemory> uploadMemory;
		std::unique_ptr<DeviceMemory> cpuWriteMemory;
		std::unique_ptr<UploadContainer> uploadContainer;
	}

	void SetThreadPool(
		ObjectManager& om, std::shared_ptr<IThreadPool>&& threadPoolArg
	) noexcept {
		om.CreateObject(threadPool, std::move(threadPoolArg), 0u);
	}

	void InitGraphicsQueue(
		ObjectManager& om, VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex,
		std::uint32_t bufferCount
	) {
		om.CreateObject(graphicsQueue, { queue }, 1u);
		om.CreateObject(graphicsCmdBuffer, { logicalDevice, queueIndex, bufferCount }, 1u);
		om.CreateObject(graphicsSyncObjects, { logicalDevice, bufferCount, true }, 1u);
	}

	void InitTransferQueue(
		ObjectManager& om, VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex
	) {
		om.CreateObject(transferQueue, { queue }, 1u);
		om.CreateObject(
			transferCmdBuffer, { .device = logicalDevice, .queueIndex = queueIndex }, 1u
		);
		om.CreateObject(transferSyncObjects, { .device = logicalDevice }, 1u);
	}

	void InitComputeQueue(
		ObjectManager& om, VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex,
		std::uint32_t bufferCount
	) {
		om.CreateObject(computeQueue, { queue }, 1u);
		om.CreateObject(computeCmdBuffer, { logicalDevice, queueIndex, bufferCount }, 1u);
		om.CreateObject(computeSyncObjects, { logicalDevice, bufferCount, true }, 1u);
	}

	void InitDisplay(ObjectManager& om) {
#ifdef TERRA_WIN32
		om.CreateObject<DisplayManagerWin32>(display, 3u);
#else
		objectManager.CreateObject<DisplayManagerVK>(display, 3u);
#endif
	}

	void InitSurface(
		ObjectManager& om, VkInstance instance, void* windowHandle, void* moduleHandle
	) {
#ifdef TERRA_WIN32
		om.CreateObject<SurfaceManagerWin32>(
			surface, { instance, windowHandle, moduleHandle }, 3u
		);
#endif
	}

	void InitDescriptorSets(
		ObjectManager& om, VkDevice logicalDevice, std::uint32_t bufferCount
	) {
		om.CreateObject(graphicsDescriptorSet, { logicalDevice, bufferCount }, 1u);
		om.CreateObject(computeDescriptorSet, { logicalDevice, bufferCount }, 1u);
	}

	void InitRenderEngine(
		ObjectManager& om, VkDevice logicalDevice, RenderEngineType engineType,
		std::uint32_t bufferCount, QueueIndices3 queueIndices
	) {
		switch (engineType) {
		case RenderEngineType::IndirectDraw: {
			om.CreateObject<RenderEngineIndirectDraw>(
				renderEngine, { logicalDevice, bufferCount, queueIndices }, 1u
				);
			break;
		}
		case RenderEngineType::IndividualDraw: {
			om.CreateObject<RenderEngineIndividualDraw>(
				renderEngine, { logicalDevice, queueIndices }, 1u
				);
			break;
		}
		case RenderEngineType::MeshDraw: {
			throw Exception("No Support", "Mesh Shader is not supported yet.");
			break;
		}
		}
	}

	void SetSharedData(
		ObjectManager& om, std::shared_ptr<ISharedDataContainer>&& sharedDataArg
	) noexcept {
		om.CreateObject(sharedData, std::move(sharedDataArg), 0u);
	}

	void InitResources(
		ObjectManager& om, VkPhysicalDevice physicalDevice, VkDevice logicalDevice
	) {
		om.CreateObject(
			Resources::cpuWriteMemory,
			{ logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
			2u
		);
		om.CreateObject(
			Resources::uploadMemory,
			{ logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
			2u
		);
		om.CreateObject(
			Resources::gpuOnlyMemory,
			{ logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
			2u
		);

		om.CreateObject(Resources::uploadContainer, 0u);
	}
}
