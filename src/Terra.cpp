#include <Terra.hpp>
#ifdef TERRA_WIN32
#include <DisplayManagerWin32.hpp>
#include <SurfaceManagerWin32.hpp>
#else
#include <DisplayManagerVK.hpp>
#endif

#include <CameraManager.hpp>

namespace Terra {
	std::shared_ptr<IThreadPool> threadPool;
	std::unique_ptr<DebugLayerManager> debugLayer;
	std::unique_ptr<CommandPoolManager> graphicsCmdPool;
	std::unique_ptr<CommandPoolManager> copyCmdPool;
	std::unique_ptr<DeviceManager> device;
	std::unique_ptr<InstanceManager> vkInstance;
	std::unique_ptr<GraphicsQueueManager> graphicsQueue;
	std::unique_ptr<CopyQueueManager> copyQueue;
	std::unique_ptr<SwapChainManager> swapChain;
	std::unique_ptr<IDisplayManager> display;
	std::unique_ptr<ISurfaceManager> surface;
	std::unique_ptr<ResourceBuffer> vertexBuffer;
	std::unique_ptr<ResourceBuffer> indexBuffer;
	std::unique_ptr<HostAccessibleBuffers> uniformBuffer;
	std::unique_ptr<ViewportAndScissorManager> viewportAndScissor;
	std::unique_ptr<RenderPassManager> renderPass;
	std::unique_ptr<ModelContainer> modelContainer;
	std::unique_ptr<DescriptorSetManager> descriptorSet;
	std::unique_ptr<TextureStorage> textureStorage;
	std::unique_ptr<CameraManager> cameraManager;
	std::unique_ptr<DepthBuffer> depthBuffer;
	std::shared_ptr<ISharedDataContainer> sharedData;

	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg) noexcept {
		threadPool = std::move(threadPoolArg);
	}

	void InitDebugLayer(VkInstance instance) {
		debugLayer = std::make_unique<DebugLayerManager>(instance);
	}

	void InitGraphicsCmdPool(
		VkDevice logicalDevice, size_t queueIndex, std::uint32_t bufferCount
	) {
		graphicsCmdPool = std::make_unique<CommandPoolManager>(
			logicalDevice, queueIndex, bufferCount
			);
	}

	void InitCopyCmdPool(
		VkDevice logicalDevice, size_t queueIndex
	) {
		copyCmdPool = std::make_unique<CommandPoolManager>(
			logicalDevice, queueIndex, 1u
			);
	}

	void InitDevice() {
		device = std::make_unique<DeviceManager>();
	}

	void InitVkInstance(const char* appName) {
		vkInstance = std::make_unique<InstanceManager>(appName);
	}

	void InitGraphicsQueue(
		VkDevice logicalDevice, VkQueue queue, std::uint32_t bufferCount
	) {
		graphicsQueue = std::make_unique<GraphicsQueueManager>(
			logicalDevice, queue, bufferCount
			);
	}

	void InitCopyQueue(
		VkDevice logicalDevice, VkQueue queue
	) {
		copyQueue = std::make_unique<CopyQueueManager>(logicalDevice, queue);
	}

	void InitSwapChain(
		const SwapChainManagerCreateInfo& swapCreateInfo,
		VkQueue presentQueue, size_t queueFamilyIndex
	) {
		swapChain = std::make_unique<SwapChainManager>(
			swapCreateInfo, presentQueue, queueFamilyIndex
			);
	}

	void InitDisplay() {
#ifdef TERRA_WIN32
		display = std::make_unique<DisplayManagerWin32>();
#else
		display = std::make_unique<DisplayManagerVK>();
#endif
	}

	void InitSurface(VkInstance instance, void* windowHandle, void* moduleHandle) {
#ifdef TERRA_WIN32
		surface = std::make_unique<SurfaceManagerWin32>(
			instance, windowHandle, moduleHandle
			);
#endif
	}

	void InitVertexBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	) {
		vertexBuffer = std::make_unique<ResourceBuffer>(
			logicalDevice, physicalDevice, queueFamilyIndices, BufferType::Vertex
			);
	}

	void InitIndexBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	) {
		indexBuffer = std::make_unique<ResourceBuffer>(
			logicalDevice, physicalDevice, queueFamilyIndices, BufferType::Index
			);
	}

	void InitUniformBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice
	) {
		uniformBuffer =
			std::make_unique<HostAccessibleBuffers>(logicalDevice, physicalDevice);
	}

	void InitViewportAndScissor(std::uint32_t width, std::uint32_t height) {
		viewportAndScissor = std::make_unique<ViewportAndScissorManager>(
			width, height
			);
	}

	void InitRenderPass(
		VkDevice logicalDevice,
		VkFormat swapChainFormat, VkFormat depthFormat
	) {
		renderPass = std::make_unique<RenderPassManager>(
			logicalDevice, swapChainFormat, depthFormat
			);
	}

	void InitModelContainer(
		const std::string& shaderPath,
		VkDevice logicalDevice
	) {
		modelContainer = std::make_unique<ModelContainer>(shaderPath, logicalDevice);
	}

	void InitDescriptorSet(VkDevice logicalDevice) {
		descriptorSet = std::make_unique<DescriptorSetManager>(logicalDevice);
	}

	void InitTextureStorage(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	) {
		textureStorage = std::make_unique<TextureStorage>(
			logicalDevice, physicalDevice,
			queueFamilyIndices
			);
	}

	void InitCameraManager() {
		cameraManager = std::make_unique<CameraManager>();
	}

	void InitDepthBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	) {
		depthBuffer = std::make_unique<DepthBuffer>(
			logicalDevice, physicalDevice, queueFamilyIndices
			);
	}

	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg) noexcept {
		sharedData = std::move(sharedDataArg);
	}
}
