#include <array>

#include <RendererVK.hpp>
#include <Terra.hpp>

RendererVK::RendererVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount, RenderEngineType engineType
) : m_appName{appName},
	m_bufferCount{ bufferCount }, m_width{ width }, m_height{ height } {

	assert(bufferCount >= 1u && "BufferCount must not be zero.");
	assert(windowHandle && moduleHandle && "Invalid Window or WindowModule Handle.");

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

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::GraphicsQueue
	);

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

	auto [transferQueueHandle, transferQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::TransferQueue
	);

	Terra::InitTransferQueue(
		m_objectManager, transferQueueHandle, logicalDevice, transferQueueFamilyIndex
	);

	auto [computeQueueHandle, computeQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::ComputeQueue
	);

	Terra::InitRenderEngine(
		m_objectManager, logicalDevice, engineType, bufferCount,
		{ transferQueueFamilyIndex, graphicsQueueFamilyIndex, computeQueueFamilyIndex }
	);
	Terra::renderEngine->ResizeViewportAndScissor(width, height);
	Terra::renderEngine->CreateRenderPass(logicalDevice, Terra::swapChain->GetSwapFormat());

	Terra::InitComputeQueue(
		m_objectManager, computeQueueHandle, logicalDevice, computeQueueFamilyIndex,
		bufferCount
	);

	Terra::InitDescriptorSets(m_objectManager, logicalDevice, bufferCount);

	m_objectManager.CreateObject(
		Terra::textureStorage, {
			logicalDevice, physicalDevice,
			QueueIndicesTG{transferQueueFamilyIndex, graphicsQueueFamilyIndex}
		}, 1u
	);

	const bool modelDataNoBB = engineType == RenderEngineType::IndirectDraw ? false : true;

	m_objectManager.CreateObject(
		Terra::bufferManager,
		{ logicalDevice,  bufferCount,
			QueueIndicesCG{computeQueueFamilyIndex, graphicsQueueFamilyIndex},
			modelDataNoBB
		},
		1u
	);

	m_objectManager.CreateObject(Terra::cameraManager, 0u);
	Terra::cameraManager->SetSceneResolution(width, height);
}

void RendererVK::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept {
	Terra::renderEngine->SetBackgroundColour(colourVector);
}

void RendererVK::AddModelSet(
	std::vector<std::shared_ptr<IModel>>&& models, const std::wstring& fragmentShader
) {
	Terra::renderEngine->RecordModelDataSet(models, fragmentShader + L".spv");
	Terra::bufferManager->AddOpaqueModels(std::move(models));
}

void RendererVK::AddMeshletModelSet(
	std::vector<MeshletModel>&& meshletModels, const std::wstring& pixelShader
) {

}

void RendererVK::AddModelInputs(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) {
	Terra::renderEngine->AddGVerticesAndIndices(
		Terra::device->GetLogicalDevice(), std::move(gVertices), std::move(gIndices)
	);
}

void RendererVK::AddModelInputs(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
	std::vector<std::uint32_t>&& gPrimIndices
) {

}

void RendererVK::Update() {
	Terra::swapChain->AcquireNextImageIndex(Terra::graphicsSyncObjects->GetFrontSemaphore());
	const size_t imageIndex = Terra::swapChain->GetNextImageIndex();

	Terra::renderEngine->UpdateModelBuffers(static_cast<VkDeviceSize>(imageIndex));
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

		Terra::renderEngine->CleanUpDepthBuffer();
		Terra::renderEngine->CreateDepthBuffer(device, width, height);

		VkSurfaceFormatKHR surfaceFormat = Terra::swapChain->GetSurfaceFormat();
		bool hasSwapFormatChanged = Terra::swapChain->HasSurfaceFormatChanged(surfaceFormat);

		if (hasSwapFormatChanged)
			Terra::renderEngine->CreateRenderPass(
				Terra::device->GetLogicalDevice(), Terra::swapChain->GetSwapFormat()
			);

		Terra::swapChain->ResizeSwapchain(
			device, Terra::surface->GetSurface(), width, height,
			Terra::renderEngine->GetRenderPass(), Terra::renderEngine->GetDepthImageView(),
			surfaceFormat
		);

		Terra::renderEngine->ResizeViewportAndScissor(width, height);

		Terra::cameraManager->SetSceneResolution(width, height);
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
	Terra::renderEngine->BindResourcesToMemory(logicalDevice);
	Terra::textureStorage->BindMemories(logicalDevice);

	Terra::renderEngine->CreateDepthBuffer(logicalDevice, m_width, m_height);
	Terra::swapChain->CreateFramebuffers(
		logicalDevice,
		Terra::renderEngine->GetRenderPass(), Terra::renderEngine->GetDepthImageView(),
		m_width, m_height
	);

	// Async Copy
	std::atomic_size_t works = 0u;

	Terra::Resources::uploadContainer->CopyData(works);
	Terra::renderEngine->CopyData();

	while (works != 0u);

	// Upload to GPU
	Terra::transferCmdBuffer->ResetFirstBuffer();
	const VkCommandBuffer transferCmdBuffer = Terra::transferCmdBuffer->GetFirstCommandBuffer();

	Terra::renderEngine->RecordCopy(transferCmdBuffer);
	Terra::textureStorage->RecordUploads(transferCmdBuffer);

	Terra::textureStorage->ReleaseOwnerships(transferCmdBuffer);
	Terra::renderEngine->ReleaseOwnership(transferCmdBuffer);

	Terra::transferCmdBuffer->CloseFirstBuffer();

	Terra::transferQueue->SubmitCommandBuffer(
		transferCmdBuffer, Terra::transferSyncObjects->GetFrontFence()
	);
	Terra::transferSyncObjects->WaitForFrontFence();
	Terra::transferSyncObjects->ResetFrontFence();

	// Transition Images to Fragment Optimal
	Terra::graphicsCmdBuffer->ResetFirstBuffer();

	const VkCommandBuffer graphicsCmdBuffer = Terra::graphicsCmdBuffer->GetFirstCommandBuffer();

	Terra::textureStorage->AcquireOwnerShips(graphicsCmdBuffer);
	Terra::renderEngine->AcquireOwnerShipGraphics(graphicsCmdBuffer);

	Terra::textureStorage->TransitionImages(graphicsCmdBuffer);

	Terra::graphicsCmdBuffer->CloseFirstBuffer();

	Terra::graphicsQueue->SubmitCommandBuffer(
		graphicsCmdBuffer, Terra::graphicsSyncObjects->GetFrontFence()
	);
	Terra::graphicsSyncObjects->WaitForFrontFence();
	Terra::graphicsSyncObjects->ResetFrontFence();

	// Compute
	Terra::computeCmdBuffer->ResetFirstBuffer();

	const VkCommandBuffer computeCmdBuffer = Terra::computeCmdBuffer->GetFirstCommandBuffer();

	Terra::renderEngine->AcquireOwnerShipCompute(computeCmdBuffer);

	Terra::computeCmdBuffer->CloseFirstBuffer();

	Terra::computeQueue->SubmitCommandBuffer(
		computeCmdBuffer, Terra::computeSyncObjects->GetFrontFence()
	);
	Terra::computeSyncObjects->WaitForFrontFence();
	Terra::computeSyncObjects->ResetFrontFence();

	Terra::textureStorage->SetDescriptorLayouts();

	Terra::graphicsDescriptorSet->CreateDescriptorSets(logicalDevice);
	Terra::computeDescriptorSet->CreateDescriptorSets(logicalDevice);

	Terra::renderEngine->ConstructPipelines(m_bufferCount);

	// Cleanup Upload Buffers
	Terra::Resources::uploadContainer.reset();
	Terra::renderEngine->ReleaseUploadResources();
	Terra::textureStorage->ReleaseUploadBuffers();
	Terra::Resources::uploadMemory.reset();
}

size_t RendererVK::AddTexture(
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
