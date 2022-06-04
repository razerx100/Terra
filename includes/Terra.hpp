#ifndef TERRA_HPP_
#define TERRA_HPP_
#include <IThreadPool.hpp>
#include <DebugLayerManager.hpp>
#include <CommandPoolManager.hpp>
#include <DeviceManager.hpp>
#include <VKInstanceManager.hpp>
#include <GraphicsQueueManager.hpp>
#include <ISurfaceManager.hpp>
#include <SwapChainManager.hpp>
#include <IDisplayManager.hpp>
#include <CopyQueueManager.hpp>
#include <ResourceBuffer.hpp>
#include <ViewportAndScissorManager.hpp>
#include <RenderPassManager.hpp>
#include <DescriptorSetManager.hpp>
#include <TextureStorage.hpp>
#include <DepthBuffer.hpp>
#include <memory>

#include <ModelContainer.hpp>
#include <ISharedDataContainer.hpp>
#include <CameraManager.hpp>

namespace Terra {
	// Variables
	extern std::shared_ptr<IThreadPool> threadPool;
	extern std::unique_ptr<DebugLayerManager> debugLayer;
	extern std::unique_ptr<CommandPoolManager> graphicsCmdPool;
	extern std::unique_ptr<CommandPoolManager> copyCmdPool;
	extern std::unique_ptr<DeviceManager> device;
	extern std::unique_ptr<InstanceManager> vkInstance;
	extern std::unique_ptr<GraphicsQueueManager> graphicsQueue;
	extern std::unique_ptr<CopyQueueManager> copyQueue;
	extern std::unique_ptr<SwapChainManager> swapChain;
	extern std::unique_ptr<IDisplayManager> display;
	extern std::unique_ptr<ISurfaceManager> surface;
	extern std::unique_ptr<ResourceBuffer> vertexBuffer;
	extern std::unique_ptr<ResourceBuffer> indexBuffer;
	extern std::unique_ptr<HostAccessibleBuffers> uniformBuffer;
	extern std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	extern std::unique_ptr<RenderPassManager> renderPass;
	extern std::unique_ptr<ModelContainer> modelContainer;
	extern std::unique_ptr<DescriptorSetManager> descriptorSet;
	extern std::unique_ptr<TextureStorage> textureStorage;
	extern std::unique_ptr<CameraManager> cameraManager;
	extern std::unique_ptr<DepthBuffer> depthBuffer;
	extern std::shared_ptr<ISharedDataContainer> sharedData;

	// Initialization functions
	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg) noexcept;
	void InitDebugLayer(VkInstance instance);
	void InitGraphicsCmdPool(
		VkDevice logicalDevice, size_t queueIndex, std::uint32_t bufferCount
	);
	void InitCopyCmdPool(
		VkDevice logicalDevice, size_t queueIndex
	);
	void InitDevice();
	void InitVkInstance(const char* appName);
	void InitGraphicsQueue(
		VkDevice logicalDevice, VkQueue queue, std::uint32_t bufferCount
	);
	void InitCopyQueue(
		VkDevice logicalDevice, VkQueue queue
	);
	void InitSwapChain(
		const SwapChainManagerCreateInfo& swapCreateInfo,
		VkQueue presentQueue, size_t queueFamilyIndex
	);
	void InitDisplay();
	void InitSurface(VkInstance instance, void* windowHandle, void* moduleHandle);
	void InitVertexBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void InitIndexBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void InitUniformBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice
	);
	void InitViewportAndScissor(std::uint32_t width, std::uint32_t height);
	void InitRenderPass(
		VkDevice logicalDevice,
		VkFormat swapChainFormat, VkFormat depthFormat
	);
	void InitModelContainer(
		const std::string& shaderPath,
		VkDevice logicalDevice
	);
	void InitDescriptorSet(VkDevice logicalDevice);
	void InitTextureStorage(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void InitCameraManager();
	void InitDepthBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg) noexcept;
}
#endif
