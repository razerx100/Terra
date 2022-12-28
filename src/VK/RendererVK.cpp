#include <array>

#include <RendererVK.hpp>
#include <Terra.hpp>

RendererVK::RendererVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount
) : m_appName{appName},
	m_bufferCount{ bufferCount }, m_width{ width }, m_height{ height },
	m_graphicsQueueIndex{ 0u }, m_computeQueueIndex{ 0u }, m_copyQueueIndex{ 0u } {

	assert(bufferCount >= 1u && "BufferCount must not be zero.");
	assert(windowHandle && moduleHandle && "Invalid Window or WindowModule Handle.");

	m_objectManager.CreateObject(Terra::viewportAndScissor, { width, height }, 0u);

	Terra::InitDisplay(m_objectManager);

	m_objectManager.CreateObject(Terra::vkInstance, { appName }, 5u);
	Terra::vkInstance->AddExtensionNames(Terra::display->GetRequiredExtensions());
	Terra::vkInstance->CreateInstance();

	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();

#ifdef _DEBUG
	m_objectManager.CreateObject(Terra::debugLayer, { vkInstance }, 4u);
#endif

#ifdef TERRA_WIN32
	Terra::InitSurface(m_objectManager, vkInstance, windowHandle, moduleHandle);
#endif

	m_objectManager.CreateObject(Terra::device, 3u);

	VkSurfaceKHR vkSurface = Terra::surface->GetSurface();

	Terra::device->FindPhysicalDevice(vkInstance, vkSurface);
	Terra::device->CreateLogicalDevice();

	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	_vkResourceView::SetBufferAlignments(physicalDevice);

	Terra::InitResources(m_objectManager, physicalDevice, logicalDevice);
	Terra::InitVertexManager(m_objectManager, logicalDevice);

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::GraphicsQueue
	);
	m_graphicsQueueIndex = graphicsQueueFamilyIndex;

	SwapChainManager::Args swapArguments{
		.device = logicalDevice,
		.surface = vkSurface,
		.surfaceInfo = QuerySurfaceCapabilities(physicalDevice, vkSurface),
		.width = width,
		.height = height,
		.bufferCount = bufferCount,
		// Graphics and Present queues should be the same
		.presentQueue = graphicsQueueHandle
	};

	m_objectManager.CreateObject(Terra::swapChain, swapArguments, 1u);

	Terra::InitGraphicsQueue(
		m_objectManager, graphicsQueueHandle, logicalDevice, graphicsQueueFamilyIndex,
		bufferCount
	);

	auto [copyQueueHandle, copyQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::TransferQueue
	);
	m_copyQueueIndex = copyQueueFamilyIndex;

	Terra::InitCopyQueue(
		m_objectManager, copyQueueHandle, logicalDevice, copyQueueFamilyIndex
	);

	auto [computeQueueHandle, computeQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::ComputeQueue
	);
	m_computeQueueIndex = computeQueueFamilyIndex;

	Terra::InitRenderEngine(
		m_objectManager, logicalDevice, bufferCount,
		ResolveQueueIndices(
			computeQueueFamilyIndex, graphicsQueueFamilyIndex, copyQueueFamilyIndex
		)
	);

	Terra::InitComputeQueue(
		m_objectManager, computeQueueHandle, logicalDevice, computeQueueFamilyIndex,
		bufferCount
	);

	m_objectManager.CreateObject(Terra::depthBuffer, { logicalDevice }, 0u);
	Terra::depthBuffer->AllocateForMaxResolution(logicalDevice, 7680u, 4320u);

	m_objectManager.CreateObject(
		Terra::renderPass,
		{
			logicalDevice, Terra::swapChain->GetSwapFormat(),
			Terra::depthBuffer->GetDepthFormat()
		},
		0u
	);

	Terra::InitDescriptorSets(m_objectManager, logicalDevice, bufferCount);

	m_objectManager.CreateObject(Terra::textureStorage, { logicalDevice, physicalDevice }, 1u);
	m_objectManager.CreateObject(
		Terra::bufferManager,
		{ logicalDevice,  bufferCount, ResolveQueueIndices(
				computeQueueFamilyIndex, graphicsQueueFamilyIndex
			)
		},
		1u
	);

	m_objectManager.CreateObject(Terra::cameraManager, 0u);
	Terra::cameraManager->SetSceneResolution(width, height);
}

void RendererVK::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept {
	Terra::renderEngine->SetBackgroundColour(colourVector);
}

void RendererVK::SubmitModels(std::vector<std::shared_ptr<IModel>>&& models) {
	Terra::renderEngine->RecordModelDataSet(models, L"FragmentShader.spv");
	Terra::bufferManager->AddOpaqueModels(std::move(models));
}

void RendererVK::SubmitModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	Terra::vertexManager->AddGlobalVertices(
		Terra::device->GetLogicalDevice(),
		std::move(vertices), vertexBufferSize, std::move(indices), indexBufferSize
	);
}

void RendererVK::Update() {
	Terra::swapChain->AcquireNextImageIndex(Terra::graphicsSyncObjects->GetFrontSemaphore());
	const size_t imageIndex = Terra::swapChain->GetNextImageIndex();

	Terra::bufferManager->Update(static_cast<VkDeviceSize>(imageIndex));
}

void RendererVK::Render() {
	const size_t imageIndex = Terra::swapChain->GetNextImageIndex();
	const VkCommandBuffer graphicsCommandBuffer = Terra::graphicsCmdBuffer->GetCommandBuffer(
		imageIndex
	);

	Terra::renderEngine->ExecutePreRenderStage(graphicsCommandBuffer, imageIndex);
	Terra::renderEngine->RecordDrawCommands(graphicsCommandBuffer, imageIndex);
	Terra::renderEngine->Present(graphicsCommandBuffer, imageIndex);
	Terra::renderEngine->ExecutePostRenderStage();
}

void RendererVK::Resize(std::uint32_t width, std::uint32_t height) {
	if (width != m_width || height != m_height) {
		m_width = width;
		m_height = height;

		VkDevice device = Terra::device->GetLogicalDevice();

		vkDeviceWaitIdle(device);

		Terra::depthBuffer->CleanUp();
		Terra::depthBuffer->CreateDepthBuffer(device, width, height);

		bool hasSwapFormatChanged = false;
		Terra::swapChain->ResizeSwapchain(
			device,
			Terra::surface->GetSurface(),
			width, height,
			Terra::renderPass->GetRenderPass(), Terra::depthBuffer->GetDepthImageView(),
			hasSwapFormatChanged
		);

		Terra::viewportAndScissor->Resize(width, height);

		Terra::cameraManager->SetSceneResolution(width, height);

		if (hasSwapFormatChanged)
			Terra::renderPass->CreateRenderPass(
				Terra::device->GetLogicalDevice(),
				Terra::swapChain->GetSwapFormat(), Terra::depthBuffer->GetDepthFormat()
			);
	}
}

Renderer::Resolution RendererVK::GetFirstDisplayCoordinates() const {
	auto [width, height] = Terra::display->GetDisplayResolution(
		Terra::device->GetPhysicalDevice(), 0u
	);

	return { width, height };
}

void RendererVK::SetShaderPath(const wchar_t* path) noexcept {
	Terra::renderEngine->SetShaderPath(path);
}

void RendererVK::ProcessData() {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	// Create Buffers
	Terra::bufferManager->CreateBuffers(logicalDevice);
	Terra::renderEngine->CreateBuffers(logicalDevice);

	// Allocate Memory
	Terra::Resources::gpuOnlyMemory->AllocateMemory(logicalDevice);
	Terra::Resources::uploadMemory->AllocateMemory(logicalDevice);
	Terra::Resources::cpuWriteMemory->AllocateMemory(logicalDevice);

	// Map cpu memories
	Terra::Resources::uploadMemory->MapMemoryToCPU(logicalDevice);
	Terra::Resources::cpuWriteMemory->MapMemoryToCPU(logicalDevice);

	// Set Upload Memory Start
	Terra::Resources::uploadContainer->SetMemoryStart(
		Terra::Resources::uploadMemory->GetMappedCPUPtr()
	);

	// Bind Buffers to memory
	Terra::bufferManager->BindResourceToMemory(logicalDevice);
	Terra::vertexManager->BindResourceToMemory(logicalDevice);
	Terra::renderEngine->BindResourcesToMemory(logicalDevice);
	Terra::textureStorage->BindMemories(logicalDevice);

	Terra::depthBuffer->CreateDepthBuffer(logicalDevice, m_width, m_height);
	Terra::swapChain->CreateFramebuffers(
		logicalDevice,
		Terra::renderPass->GetRenderPass(), Terra::depthBuffer->GetDepthImageView(),
		m_width, m_height
	);

	// Async Copy
	std::atomic_size_t works = 0u;

	Terra::Resources::uploadContainer->CopyData(works);
	Terra::renderEngine->CopyData();

	while (works != 0u);

	// Check if queue families are the same
	bool copyAndGraphics = m_copyQueueIndex == m_graphicsQueueIndex ? true : false;
	bool copyAndCompute = m_copyQueueIndex == m_computeQueueIndex ? true : false;

	// Upload to GPU
	Terra::copyCmdBuffer->ResetFirstBuffer();
	const VkCommandBuffer copyCmdBuffer = Terra::copyCmdBuffer->GetFirstCommandBuffer();

	Terra::vertexManager->RecordCopy(copyCmdBuffer);
	Terra::renderEngine->RecordCopy(copyCmdBuffer);
	Terra::textureStorage->RecordUploads(copyCmdBuffer);

	// If copy and graphics queues are different release ownership from copy
	if (!copyAndGraphics) {
		Terra::vertexManager->ReleaseOwnerships(
			copyCmdBuffer, m_copyQueueIndex, m_graphicsQueueIndex
		);
		Terra::textureStorage->ReleaseOwnerships(
			copyCmdBuffer, m_copyQueueIndex, m_graphicsQueueIndex
		);
	}

	// If copy and compute queues are different release ownership from copy
	if (!copyAndCompute)
		Terra::renderEngine->ReleaseOwnership(
			copyCmdBuffer, m_copyQueueIndex, m_computeQueueIndex
		);

	Terra::copyCmdBuffer->CloseFirstBuffer();

	Terra::copyQueue->SubmitCommandBuffer(
		copyCmdBuffer, Terra::copySyncObjects->GetFrontFence()
	);
	Terra::copySyncObjects->WaitForFrontFence();
	Terra::copySyncObjects->ResetFrontFence();

	// Transition Images to Fragment Optimal
	Terra::graphicsCmdBuffer->ResetFirstBuffer();

	const VkCommandBuffer graphicsCmdBuffer = Terra::graphicsCmdBuffer->GetFirstCommandBuffer();

	Terra::textureStorage->TransitionImages(graphicsCmdBuffer);

	// If copy and graphics queues are different release ownership from copy
	if (!copyAndGraphics) {
		Terra::vertexManager->AcquireOwnerShips(
			graphicsCmdBuffer, m_copyQueueIndex, m_graphicsQueueIndex
		);
		Terra::textureStorage->AcquireOwnerShips(
			graphicsCmdBuffer, m_copyQueueIndex, m_graphicsQueueIndex
		);
	}

	Terra::graphicsCmdBuffer->CloseFirstBuffer();

	Terra::graphicsQueue->SubmitCommandBuffer(
		graphicsCmdBuffer, Terra::graphicsSyncObjects->GetFrontFence()
	);
	Terra::graphicsSyncObjects->WaitForFrontFence();
	Terra::graphicsSyncObjects->ResetFrontFence();

	if (!copyAndCompute) {
		// Transfer ownership
		Terra::computeCmdBuffer->ResetFirstBuffer();

		const VkCommandBuffer computeCmdBuffer = Terra::computeCmdBuffer->GetFirstCommandBuffer();

		Terra::renderEngine->AcquireOwnerShip(
			computeCmdBuffer, m_copyQueueIndex, m_computeQueueIndex
		);

		Terra::computeCmdBuffer->CloseFirstBuffer();

		Terra::computeQueue->SubmitCommandBuffer(
			computeCmdBuffer, Terra::computeSyncObjects->GetFrontFence()
		);
		Terra::computeSyncObjects->WaitForFrontFence();
		Terra::computeSyncObjects->ResetFrontFence();
	}

	Terra::textureStorage->SetDescriptorLayouts();

	Terra::graphicsDescriptorSet->CreateDescriptorSets(logicalDevice);
	Terra::computeDescriptorSet->CreateDescriptorSets(logicalDevice);

	Terra::renderEngine->ConstructPipelines(m_bufferCount);

	// Cleanup Upload Buffers
	Terra::Resources::uploadContainer.reset();
	Terra::vertexManager->ReleaseUploadResources();
	Terra::renderEngine->ReleaseUploadResources();
	Terra::textureStorage->ReleaseUploadBuffers();
	Terra::Resources::uploadMemory.reset();
}

size_t RendererVK::RegisterResource(
	std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
) {
	return Terra::textureStorage->AddTexture(
		Terra::device->GetLogicalDevice(), std::move(textureData), width, height
	);
}

void RendererVK::SetThreadPool(std::shared_ptr<IThreadPool> threadPoolArg) noexcept {
	Terra::SetThreadPool(m_objectManager, std::move(threadPoolArg));
}

void RendererVK::SetSharedDataContainer(
	std::shared_ptr<ISharedDataContainer> sharedData
) noexcept {
	Terra::SetSharedData(m_objectManager, std::move(sharedData));
}

void RendererVK::WaitForAsyncTasks() {
	vkDeviceWaitIdle(Terra::device->GetLogicalDevice());
}
