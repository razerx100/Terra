#include <array>

#include <RendererVK.hpp>
#include <Terra.hpp>

RendererVK::RendererVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount
) : m_backgroundColour{}, m_appName(appName),
	m_bufferCount(bufferCount), m_width(width), m_height(height) {

	m_backgroundColour = {
		{0.0001f, 0.0001f, 0.0001f, 0.0001f }
	};

	Terra::InitViewportAndScissor(width, height);

	Terra::InitDisplay();

	Terra::InitVkInstance(appName);
	Terra::vkInstance->AddExtensionNames(Terra::display->GetRequiredExtensions());
	Terra::vkInstance->CreateInstance();

	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();

#ifdef _DEBUG
	Terra::InitDebugLayer(vkInstance);
#endif

#ifdef TERRA_WIN32
	Terra::InitSurface(vkInstance, windowHandle, moduleHandle);
#endif

	Terra::InitDevice();

	VkSurfaceKHR vkSurface = Terra::surface->GetSurface();

	Terra::device->FindPhysicalDevice(
		vkInstance, vkSurface
	);
	Terra::device->CreateLogicalDevice();

	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	Terra::InitResources(physicalDevice, logicalDevice);

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::GraphicsQueue
	);

	SwapChainManagerCreateInfo swapCreateInfo{};
	swapCreateInfo.device = logicalDevice;
	swapCreateInfo.surfaceInfo = QuerySurfaceCapabilities(physicalDevice, vkSurface);
	swapCreateInfo.surface = vkSurface;
	swapCreateInfo.width = width;
	swapCreateInfo.height = height;
	swapCreateInfo.bufferCount = bufferCount;

	// Graphics and Present queues should be the same
	Terra::InitSwapChain(swapCreateInfo, graphicsQueueHandle);

	Terra::InitGraphicsQueue(
		graphicsQueueHandle, logicalDevice, graphicsQueueFamilyIndex, bufferCount
	);

	auto [copyQueueHandle, copyQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::TransferQueue
	);

	Terra::InitCopyQueue(copyQueueHandle, logicalDevice, copyQueueFamilyIndex);

	// Need to handle compute queue for Resource Sharing
	auto [computeQueueHandle, computeQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::ComputeQueue
	);

	Terra::InitComputeQueue(computeQueueHandle, logicalDevice, computeQueueFamilyIndex);

	std::vector<std::uint32_t> copyAndGfxFamilyIndices =
		DeviceManager::ResolveQueueIndices(copyQueueFamilyIndex, graphicsQueueFamilyIndex);

	Terra::InitDepthBuffer(logicalDevice, copyAndGfxFamilyIndices);
	Terra::depthBuffer->AllocateForMaxResolution(logicalDevice, 7680u, 4320u);

	Terra::InitRenderPass(
		logicalDevice, Terra::swapChain->GetSwapFormat(), Terra::depthBuffer->GetDepthFormat()
	);

	Terra::InitDescriptorSet(logicalDevice, bufferCount);

	Terra::InitTextureStorage(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	Terra::InitModelManager(logicalDevice, copyAndGfxFamilyIndices, bufferCount);

	Terra::InitCameraManager();
	Terra::cameraManager->SetSceneResolution(width, height);
}

RendererVK::~RendererVK() noexcept {
	Terra::cameraManager.reset();
	Terra::viewportAndScissor.reset();
	Terra::modelManager.reset();
	Terra::descriptorSet.reset();
	Terra::textureStorage.reset();
	Terra::copyQueue.reset();
	Terra::copyCmdBuffer.reset();
	Terra::copySyncObjects.reset();
	Terra::renderPass.reset();
	Terra::depthBuffer.reset();
	Terra::swapChain.reset();
	Terra::CleanUpResources();
	Terra::computeCmdBuffer.reset();
	Terra::computeQueue.reset();
	Terra::computeSyncObjects.reset();
	Terra::graphicsQueue.reset();
	Terra::graphicsCmdBuffer.reset();
	Terra::graphicsSyncObjects.reset();
	Terra::display.reset();
	Terra::device.reset();
	Terra::surface.reset();
#ifdef _DEBUG
	Terra::debugLayer.reset();
#endif
	Terra::vkInstance.reset();
}

void RendererVK::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept {
	m_backgroundColour = {
		{colourVector[0], colourVector[1], colourVector[2], colourVector[3]}
	};
}

void RendererVK::SubmitModels(
	std::vector<std::shared_ptr<IModel>>&& models
) {
	Terra::modelManager->AddModels(std::move(models));
}

void RendererVK::SubmitModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	Terra::modelManager->AddModelInputs(
		Terra::device->GetLogicalDevice(),
		std::move(vertices), vertexBufferSize, std::move(indices), indexBufferSize
	);
}

void RendererVK::Update() {

}

void RendererVK::Render() {
	Terra::graphicsSyncObjects->WaitForFrontFence();
	Terra::graphicsSyncObjects->ResetFrontFence();

	const size_t imageIndex = Terra::swapChain->GetAvailableImageIndex(
		Terra::graphicsSyncObjects->GetFrontSemaphore()
	);

	Terra::graphicsCmdBuffer->ResetBuffer(imageIndex);

	const VkCommandBuffer graphicsCommandBuffer = Terra::graphicsCmdBuffer->GetCommandBuffer(
		imageIndex
	);

	vkCmdSetViewport(
		graphicsCommandBuffer, 0u, 1u, Terra::viewportAndScissor->GetViewportRef()
	);
	vkCmdSetScissor(
		graphicsCommandBuffer, 0u, 1u, Terra::viewportAndScissor->GetScissorRef()
	);

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = m_backgroundColour;
	clearValues[1].depthStencil = { 1.f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderPass = Terra::renderPass->GetRenderPass();
	renderPassInfo.framebuffer = Terra::swapChain->GetFramebuffer(imageIndex);
	renderPassInfo.renderArea.extent = Terra::swapChain->GetSwapExtent();
	renderPassInfo.clearValueCount = static_cast<std::uint32_t>(std::size(clearValues));
	renderPassInfo.pClearValues = std::data(clearValues);

	vkCmdBeginRenderPass(graphicsCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	Terra::modelManager->BindCommands(graphicsCommandBuffer, imageIndex);

	vkCmdEndRenderPass(graphicsCommandBuffer);

	Terra::graphicsCmdBuffer->CloseBuffer(imageIndex);

	Terra::graphicsQueue->SubmitCommandBufferForRendering(
		graphicsCommandBuffer, Terra::graphicsSyncObjects->GetFrontFence(),
		Terra::graphicsSyncObjects->GetFrontSemaphore()
	);
	Terra::swapChain->PresentImage(static_cast<std::uint32_t>(imageIndex));

	Terra::graphicsSyncObjects->AdvanceSyncObjectsInQueue();
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

Renderer::Resolution RendererVK::GetDisplayCoordinates(std::uint32_t displayIndex) const {
	auto [width, height] = Terra::display->GetDisplayResolution(
		Terra::device->GetPhysicalDevice(), displayIndex
	);

	return { width, height };
}

void RendererVK::SetShaderPath(const wchar_t* path) noexcept {
	Terra::modelManager->SetShaderPath(path);
}

void RendererVK::ProcessData() {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	// Create Buffers
	Terra::modelManager->CreateBuffers(logicalDevice, m_bufferCount);

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
	Terra::modelManager->BindMemories(logicalDevice);
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
	Terra::modelManager->CopyData();

	while (works != 0u);

	// Upload to GPU
	Terra::copyCmdBuffer->ResetBuffer();
	const VkCommandBuffer copyCmdBuffer = Terra::copyCmdBuffer->GetCommandBuffer();

	Terra::modelManager->RecordUploadBuffers(copyCmdBuffer);
	Terra::textureStorage->RecordUploads(copyCmdBuffer);

	Terra::copyCmdBuffer->CloseBuffer();

	Terra::copyQueue->SubmitCommandBuffer(
		copyCmdBuffer, Terra::copySyncObjects->GetFrontFence()
	);
	Terra::copySyncObjects->WaitForFrontFence();
	Terra::copySyncObjects->ResetFrontFence();

	// Transition Images to Fragment Optimal
	Terra::graphicsCmdBuffer->ResetBuffer();
	Terra::graphicsSyncObjects->ResetFrontFence();

	const VkCommandBuffer graphicsCmdBuffer = Terra::graphicsCmdBuffer->GetCommandBuffer();

	Terra::textureStorage->TransitionImages(graphicsCmdBuffer);

	Terra::graphicsCmdBuffer->CloseBuffer();

	Terra::graphicsQueue->SubmitCommandBuffer(
		graphicsCmdBuffer, Terra::graphicsSyncObjects->GetFrontFence()
	);
	Terra::graphicsSyncObjects->WaitForFrontFence();
	// Leaving the fence in signaled state

	Terra::textureStorage->SetDescriptorLayouts();

	Terra::descriptorSet->CreateDescriptorSets(logicalDevice);

	Terra::modelManager->InitPipelines(
		logicalDevice, m_bufferCount, Terra::descriptorSet->GetDescriptorSetLayouts()
	);

	// Cleanup Upload Buffers
	Terra::Resources::uploadContainer.reset();
	Terra::modelManager->ReleaseUploadBuffers();
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
	Terra::SetThreadPool(std::move(threadPoolArg));
}

void RendererVK::SetSharedDataContainer(
	std::shared_ptr<ISharedDataContainer> sharedData
) noexcept {
	Terra::SetSharedData(std::move(sharedData));
}
