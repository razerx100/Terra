#ifdef TERRA_WIN32
#include <DisplayManagerWin32.hpp>
#include <SurfaceManagerWin32.hpp>
#else
#include <DisplayManagerVK.hpp>
#endif

#include <Terra.hpp>

namespace Terra {
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
	std::unique_ptr<RenderPipeline> renderPipeline;
	std::unique_ptr<DescriptorSetManager> graphicsDescriptorSet;
	std::unique_ptr<DescriptorSetManager> computeDescriptorSet;
	std::unique_ptr<TextureStorage> textureStorage;
	std::unique_ptr<CameraManager> cameraManager;
	std::unique_ptr<DepthBuffer> depthBuffer;
	std::shared_ptr<ISharedDataContainer> sharedData;

	namespace Resources {
		std::unique_ptr<DeviceMemory> gpuOnlyMemory;
		std::unique_ptr<DeviceMemory> uploadMemory;
		std::unique_ptr<DeviceMemory> cpuWriteMemory;
		std::unique_ptr<UploadContainer> uploadContainer;
	}

	void SetThreadPool(std::shared_ptr<IThreadPool>&& threadPoolArg) noexcept {
		threadPool = std::move(threadPoolArg);
	}

	void InitDebugLayer(VkInstance instance) {
		debugLayer = std::make_unique<DebugLayerManager>(instance);
	}

	void InitDevice() {
		device = std::make_unique<DeviceManager>();
	}

	void InitVkInstance(const char* appName) {
		vkInstance = std::make_unique<InstanceManager>(appName);
	}

	void InitGraphicsQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex,
		std::uint32_t bufferCount
	) {
		graphicsQueue = std::make_unique<VkCommandQueue>(queue);
		graphicsCmdBuffer = std::make_unique<VKCommandBuffer>(
			logicalDevice, queueIndex, bufferCount
			);
		graphicsSyncObjects = std::make_unique<VkSyncObjects>(
			logicalDevice, bufferCount, true
			);
	}

	void InitCopyQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex
	) {
		copyQueue = std::make_unique<VkCommandQueue>(queue);
		copyCmdBuffer = std::make_unique<VKCommandBuffer>(logicalDevice, queueIndex);
		copySyncObjects = std::make_unique<VkSyncObjects>(logicalDevice);
	}

	void InitComputeQueue(
		VkQueue queue, VkDevice logicalDevice, std::uint32_t queueIndex,
		std::uint32_t bufferCount
	) {
		computeQueue = std::make_unique<VkCommandQueue>(queue);
		computeCmdBuffer = std::make_unique<VKCommandBuffer>(
			logicalDevice, queueIndex, bufferCount
			);
		computeSyncObjects = std::make_unique<VkSyncObjects>(
			logicalDevice, bufferCount, true
			);
	}

	void InitSwapChain(
		const SwapChainManagerCreateInfo& swapCreateInfo, VkQueue presentQueue
	) {
		swapChain = std::make_unique<SwapChainManager>(swapCreateInfo, presentQueue);
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
		surface = std::make_unique<SurfaceManagerWin32>(instance, windowHandle, moduleHandle);
#endif
	}

	void InitViewportAndScissor(std::uint32_t width, std::uint32_t height) {
		viewportAndScissor = std::make_unique<ViewportAndScissorManager>(width, height);
	}

	void InitRenderPass(
		VkDevice logicalDevice, VkFormat swapChainFormat, VkFormat depthFormat
	) {
		renderPass = std::make_unique<RenderPassManager>(
			logicalDevice, swapChainFormat, depthFormat
			);
	}

	void InitBufferManager(
		VkDevice logicalDevice, const std::vector<std::uint32_t>& queueFamilyIndices,
		std::uint32_t bufferCount
	) {
		bufferManager = std::make_unique<BufferManager>(
			logicalDevice, queueFamilyIndices, bufferCount
			);
	}

	void InitRenderPipeline(
		VkDevice logicalDevice, const std::vector<std::uint32_t>& queueFamilyIndices,
		std::uint32_t bufferCount
	) {
		renderPipeline = std::make_unique<RenderPipeline>(
			logicalDevice, queueFamilyIndices, bufferCount
			);
	}

	void InitDescriptorSets(VkDevice logicalDevice, std::uint32_t bufferCount) {
		graphicsDescriptorSet =
			std::make_unique<DescriptorSetManager>(logicalDevice, bufferCount);
		computeDescriptorSet =
			std::make_unique<DescriptorSetManager>(logicalDevice, bufferCount);
	}

	void InitTextureStorage(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices
	) {
		textureStorage = std::make_unique<TextureStorage>(
			logicalDevice, physicalDevice, queueFamilyIndices
			);
	}

	void InitCameraManager() {
		cameraManager = std::make_unique<CameraManager>();
	}

	void InitDepthBuffer(VkDevice logicalDevice) {
		depthBuffer = std::make_unique<DepthBuffer>(logicalDevice);
	}

	void SetSharedData(std::shared_ptr<ISharedDataContainer>&& sharedDataArg) noexcept {
		sharedData = std::move(sharedDataArg);
	}

	void InitResources(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) noexcept {
		Resources::cpuWriteMemory = std::make_unique<DeviceMemory>(
			logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		Resources::uploadMemory = std::make_unique<DeviceMemory>(
			logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		Resources::gpuOnlyMemory = std::make_unique<DeviceMemory>(
			logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);

		Resources::uploadContainer = std::make_unique<UploadContainer>();
	}

	void CleanUpResources() noexcept {
		Terra::Resources::uploadContainer.reset();
		Terra::Resources::uploadMemory.reset();
		Resources::cpuWriteMemory.reset();
		Resources::gpuOnlyMemory.reset();
	}
}
