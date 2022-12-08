#include <array>
#include <PipelineConstructor.hpp>

#include <RendererVK.hpp>
#include <Terra.hpp>

RendererVK::RendererVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount
) : m_backgroundColour{}, m_appName{appName},
	m_bufferCount{ bufferCount }, m_width{ width }, m_height{ height },
	m_graphicsQueueIndex{ 0u }, m_computeQueueIndex{ 0u }, m_copyQueueIndex{ 0u } {

	assert(bufferCount >= 1u && "BufferCount must not be zero.");
	assert(windowHandle && moduleHandle && "Invalid Window or WindowModule Handle.");

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

	Terra::device->FindPhysicalDevice(vkInstance, vkSurface);
	Terra::device->CreateLogicalDevice();

	VkDevice logicalDevice = Terra::device->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	Terra::InitResources(physicalDevice, logicalDevice);

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::GraphicsQueue
	);
	m_graphicsQueueIndex = graphicsQueueFamilyIndex;

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
	m_copyQueueIndex = copyQueueFamilyIndex;

	Terra::InitCopyQueue(copyQueueHandle, logicalDevice, copyQueueFamilyIndex);

	auto [computeQueueHandle, computeQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::ComputeQueue
	);
	m_computeQueueIndex = computeQueueFamilyIndex;

	std::vector<std::uint32_t> computeAndGraphicsQueueIndices =
		DeviceManager::ResolveQueueIndices(computeQueueFamilyIndex, graphicsQueueFamilyIndex);

	Terra::InitComputeQueue(
		computeQueueHandle, logicalDevice, computeQueueFamilyIndex, bufferCount
	);

	Terra::InitDepthBuffer(logicalDevice);
	Terra::depthBuffer->AllocateForMaxResolution(logicalDevice, 7680u, 4320u);

	Terra::InitRenderPass(
		logicalDevice, Terra::swapChain->GetSwapFormat(), Terra::depthBuffer->GetDepthFormat()
	);

	Terra::InitDescriptorSets(logicalDevice, bufferCount);

	Terra::InitTextureStorage(logicalDevice, physicalDevice);
	Terra::InitBufferManager(logicalDevice,  bufferCount, computeAndGraphicsQueueIndices);
	Terra::InitRenderPipeline(logicalDevice,  bufferCount, computeAndGraphicsQueueIndices);

	Terra::InitCameraManager();
	Terra::cameraManager->SetSceneResolution(width, height);
}

RendererVK::~RendererVK() noexcept {
	Terra::cameraManager.reset();
	Terra::viewportAndScissor.reset();
	Terra::bufferManager.reset();
	Terra::renderPipeline.reset();
	Terra::computeDescriptorSet.reset();
	Terra::graphicsDescriptorSet.reset();
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

void RendererVK::SubmitModels(std::vector<std::shared_ptr<IModel>>&& models) {
	Terra::renderPipeline->RecordIndirectArguments(models);
	Terra::bufferManager->AddOpaqueModels(std::move(models));
}

void RendererVK::SubmitModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	Terra::bufferManager->AddModelInputs(
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

	// Compute Stage
	Terra::computeCmdBuffer->ResetBuffer(imageIndex);

	const VkCommandBuffer computeCommandBuffer = Terra::computeCmdBuffer->GetCommandBuffer(
		imageIndex
	);

	Terra::renderPipeline->ResetCounterBuffer(computeCommandBuffer, imageIndex);
	Terra::renderPipeline->BindComputePipeline(
		computeCommandBuffer, Terra::computeDescriptorSet->GetDescriptorSet(imageIndex)
	);
	Terra::renderPipeline->DispatchCompute(computeCommandBuffer);

	Terra::computeCmdBuffer->CloseBuffer(imageIndex);
	Terra::computeQueue->SubmitCommandBuffer(
		computeCommandBuffer, Terra::computeSyncObjects->GetFrontSemaphore()
	);

	// Garphics Stage
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

	Terra::renderPipeline->BindGraphicsPipeline(
		graphicsCommandBuffer, Terra::graphicsDescriptorSet->GetDescriptorSet(imageIndex)
	);
	Terra::bufferManager->BindVertexBuffer(graphicsCommandBuffer);
	Terra::renderPipeline->DrawModels(graphicsCommandBuffer, imageIndex);

	vkCmdEndRenderPass(graphicsCommandBuffer);

	Terra::graphicsCmdBuffer->CloseBuffer(imageIndex);

	VkSemaphore waitSemaphores[] = {
		Terra::computeSyncObjects->GetFrontSemaphore(),
		Terra::graphicsSyncObjects->GetFrontSemaphore()
	};
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	Terra::graphicsQueue->SubmitCommandBuffer(
		graphicsCommandBuffer, Terra::graphicsSyncObjects->GetFrontFence(),
		2u, waitSemaphores, waitStages
	);
	Terra::swapChain->PresentImage(static_cast<std::uint32_t>(imageIndex));

	Terra::graphicsSyncObjects->AdvanceSyncObjectsInQueue();
	Terra::computeSyncObjects->AdvanceSemaphoreInQueue();

	Terra::graphicsSyncObjects->WaitForFrontFence();
	Terra::graphicsSyncObjects->ResetFrontFence();
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
	m_shaderPath = path;
}

void RendererVK::ProcessData() {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	// Create Buffers
	Terra::bufferManager->CreateBuffers(logicalDevice);
	Terra::renderPipeline->CreateBuffers(logicalDevice);

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
	Terra::renderPipeline->BindResourceToMemory(logicalDevice);
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
	Terra::renderPipeline->CopyData();

	while (works != 0u);

	// Check if queue families are the same
	bool copyAndGraphics = m_copyQueueIndex == m_graphicsQueueIndex ? true : false;
	bool copyAndCompute = m_copyQueueIndex == m_computeQueueIndex ? true : false;

	// Upload to GPU
	Terra::copyCmdBuffer->ResetFirstBuffer();
	const VkCommandBuffer copyCmdBuffer = Terra::copyCmdBuffer->GetFirstCommandBuffer();

	Terra::bufferManager->RecordCopy(copyCmdBuffer);
	Terra::renderPipeline->RecordCopy(copyCmdBuffer);
	Terra::textureStorage->RecordUploads(copyCmdBuffer);

	// If copy and graphics queues are different release ownership from copy
	if (!copyAndGraphics) {
		Terra::bufferManager->ReleaseOwnerships(
			copyCmdBuffer, m_copyQueueIndex, m_graphicsQueueIndex
		);
		Terra::textureStorage->ReleaseOwnerships(
			copyCmdBuffer, m_copyQueueIndex, m_graphicsQueueIndex
		);
	}

	// If copy and compute queues are different release ownership from copy
	if (!copyAndCompute)
		Terra::renderPipeline->ReleaseOwnership(
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
		Terra::bufferManager->AcquireOwnerShips(
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

		const VkCommandBuffer computeCmdBuffer = Terra::graphicsCmdBuffer->GetFirstCommandBuffer();

		Terra::renderPipeline->AcquireOwnerShip(
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

	ConstructPipelines();

	// Cleanup Upload Buffers
	Terra::Resources::uploadContainer.reset();
	Terra::bufferManager->ReleaseUploadResources();
	Terra::renderPipeline->ReleaseUploadResources();
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

void RendererVK::WaitForAsyncTasks() {
	vkDeviceWaitIdle(Terra::device->GetLogicalDevice());
}

void RendererVK::ConstructPipelines() {
	VkDevice device = Terra::device->GetLogicalDevice();

	auto graphicsLayout = CreateGraphicsPipelineLayout(
		device, m_bufferCount, Terra::graphicsDescriptorSet->GetDescriptorSetLayouts()
	);
	auto graphicsPipeline = CreateGraphicsPipeline(
		device, graphicsLayout->GetLayout(), Terra::renderPass->GetRenderPass(),
		m_shaderPath
	);

	Terra::renderPipeline->AddGraphicsPipelineLayout(std::move(graphicsLayout));
	Terra::renderPipeline->AddGraphicsPipelineObject(std::move(graphicsPipeline));

	auto computeLayout = CreateComputePipelineLayout(
		device, m_bufferCount, Terra::computeDescriptorSet->GetDescriptorSetLayouts()
	);
	auto computePipeline = CreateComputePipeline(
		device, computeLayout->GetLayout(), m_shaderPath
	);

	Terra::renderPipeline->AddComputePipelineLayout(std::move(computeLayout));
	Terra::renderPipeline->AddComputePipelineObject(std::move(computePipeline));
}
