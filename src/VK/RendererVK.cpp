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

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::GraphicsQueue
	);
	Terra::InitGraphicsQueue(
		logicalDevice,
		graphicsQueueHandle,
		bufferCount
	);

	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	SwapChainManagerCreateInfo swapCreateInfo = {};
	swapCreateInfo.device = logicalDevice;
	swapCreateInfo.surfaceInfo = QuerySurfaceCapabilities(physicalDevice, vkSurface);
	swapCreateInfo.surface = vkSurface;
	swapCreateInfo.width = width;
	swapCreateInfo.height = height;
	swapCreateInfo.bufferCount = bufferCount;

	// Graphics and Present queues should be the same
	Terra::InitSwapChain(swapCreateInfo, graphicsQueueHandle, graphicsQueueFamilyIndex);

	Terra::InitGraphicsCmdPool(
		logicalDevice,
		graphicsQueueFamilyIndex,
		bufferCount
	);

	auto [copyQueueHandle, copyQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::TransferQueue
	);

	std::vector<std::uint32_t> copyAndGfxFamilyIndices;
	if (copyQueueFamilyIndex == graphicsQueueFamilyIndex)
		copyAndGfxFamilyIndices.emplace_back(
			static_cast<std::uint32_t>(graphicsQueueFamilyIndex)
		);
	else {
		copyAndGfxFamilyIndices.emplace_back(
			static_cast<std::uint32_t>(copyQueueFamilyIndex)
		);
		copyAndGfxFamilyIndices.emplace_back(
			static_cast<std::uint32_t>(graphicsQueueFamilyIndex)
		);
	}

	Terra::InitCopyQueue(logicalDevice, copyQueueHandle);

	Terra::InitCopyCmdPool(logicalDevice, copyQueueFamilyIndex);

	Terra::InitDepthBuffer(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	Terra::depthBuffer->CreateDepthBuffer(logicalDevice, width, height);

	Terra::InitRenderPass(
		logicalDevice,
		Terra::swapChain->GetSwapFormat(), Terra::depthBuffer->GetDepthFormat()
	);

	Terra::swapChain->CreateFramebuffers(
		logicalDevice,
		Terra::renderPass->GetRenderPass(), Terra::depthBuffer->GetDepthImageView(),
		width, height
	);

	Terra::InitDescriptorSet(logicalDevice);

	Terra::InitVertexBuffer(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	Terra::InitIndexBuffer(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	Terra::InitUniformBuffer(logicalDevice, physicalDevice);
	Terra::InitTextureStorage(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);

	Terra::InitCameraManager();
	Terra::cameraManager->SetSceneResolution(width, height);
}

RendererVK::~RendererVK() noexcept {
	Terra::cameraManager.reset();
	Terra::viewportAndScissor.reset();
	Terra::modelContainer.reset();
	Terra::descriptorSet.reset();
	Terra::uniformBuffer.reset();
	Terra::vertexBuffer.reset();
	Terra::indexBuffer.reset();
	Terra::textureStorage.reset();
	Terra::copyQueue.reset();
	Terra::copyCmdPool.reset();
	Terra::renderPass.reset();
	Terra::depthBuffer.reset();
	Terra::swapChain.reset();
	Terra::graphicsQueue.reset();
	Terra::graphicsCmdPool.reset();
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
	//Terra::modelContainer->AddModels(
	//	Terra::device->GetLogicalDevice(), std::move(models), std::move(modelInputs)
	//);
}

void RendererVK::SubmitModelInputs(
	std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize, size_t strideSize,
	std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
) {
	Terra::modelContainer->AddModelInputs(
		Terra::device->GetLogicalDevice(),
		std::move(vertices), vertexBufferSize, std::move(indices), indexBufferSize
	);
}

void RendererVK::Render() {
	Terra::graphicsQueue->WaitForGPU();
	Terra::graphicsQueue->ResetFence();

	size_t imageIndex = Terra::swapChain->GetAvailableImageIndex();
	Terra::graphicsCmdPool->Reset(imageIndex);

	VkCommandBuffer commandBuffer = Terra::graphicsCmdPool->GetCommandBuffer(imageIndex);

	vkCmdSetViewport(commandBuffer, 0u, 1u, Terra::viewportAndScissor->GetViewportRef());
	vkCmdSetScissor(commandBuffer, 0u, 1u, Terra::viewportAndScissor->GetScissorRef());

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = m_backgroundColour;
	clearValues[1].depthStencil = { 1.f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderPass = Terra::renderPass->GetRenderPass();
	renderPassInfo.framebuffer = Terra::swapChain->GetFramebuffer(imageIndex);
	renderPassInfo.renderArea.extent = Terra::swapChain->GetSwapExtent();
	renderPassInfo.clearValueCount = static_cast<std::uint32_t>(std::size(clearValues));
	renderPassInfo.pClearValues = std::data(clearValues);

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	Terra::modelContainer->BindCommands(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	Terra::graphicsCmdPool->Close(imageIndex);

	Terra::graphicsQueue->SubmitCommandBuffer(
		commandBuffer, Terra::swapChain->GetImageSemaphore()
	);
	Terra::swapChain->PresentImage(static_cast<std::uint32_t>(imageIndex));

	++imageIndex;
	const size_t nextFrame =
		imageIndex >= m_bufferCount ? imageIndex % m_bufferCount : imageIndex;

	Terra::swapChain->SetNextFrameIndex(nextFrame);
	Terra::graphicsQueue->SetNextFrameIndex(nextFrame);
}

void RendererVK::Resize(std::uint32_t width, std::uint32_t height) {
	if (width != m_width || height != m_height) {
		m_width = width;
		m_height = height;

		VkDevice device = Terra::device->GetLogicalDevice();

		vkDeviceWaitIdle(device);

		Terra::depthBuffer->CleanUp(device);
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

void RendererVK::WaitForAsyncTasks() {
	vkDeviceWaitIdle(
		Terra::device->GetLogicalDevice()
	);
}

void RendererVK::SetShaderPath(const char* path) noexcept {
	m_shaderPath = path;
}

void RendererVK::InitResourceBasedObjects() {
	Terra::InitModelContainer(
		m_shaderPath,
		Terra::device->GetLogicalDevice()
	);
}

void RendererVK::ProcessData() {
	VkDevice logicalDevice = Terra::device->GetLogicalDevice();

	Terra::modelContainer->CreateBuffers(logicalDevice);
	Terra::textureStorage->CreateBuffers(logicalDevice);

	// Async Copy
	std::atomic_size_t works = 0u;

	Terra::modelContainer->CopyData(works);
	Terra::textureStorage->CopyData(works);

	while (works != 0u);

	// Upload to GPU
	Terra::copyCmdPool->Reset(0u);
	VkCommandBuffer copyBuffer = Terra::copyCmdPool->GetCommandBuffer(0u);

	Terra::modelContainer->RecordUploadBuffers(logicalDevice, copyBuffer);
	Terra::textureStorage->RecordUploads(logicalDevice, copyBuffer);

	Terra::copyCmdPool->Close(0u);

	Terra::copyQueue->SubmitCommandBuffer(copyBuffer);
	Terra::copyQueue->WaitForGPU();
	Terra::copyQueue->ResetFence();

	// Transition Images to Fragment Optimal
	Terra::graphicsCmdPool->Reset(0u);

	VkCommandBuffer graphicsCmdBuffer = Terra::graphicsCmdPool->GetCommandBuffer(0u);

	Terra::textureStorage->TransitionImages(graphicsCmdBuffer);

	Terra::graphicsCmdPool->Close(0u);

	Terra::graphicsQueue->ResetFence();
	Terra::graphicsQueue->SubmitCommandBuffer(graphicsCmdBuffer);
	Terra::graphicsQueue->WaitForGPU();
	// Leaving the fence in signaled state

	Terra::textureStorage->SetDescriptorLayouts();

	Terra::descriptorSet->CreateDescriptorSets(logicalDevice);

	Terra::modelContainer->InitPipelines(
		logicalDevice,
		Terra::descriptorSet->GetDescriptorSetLayout()
	);

	// Cleanup Upload Buffers
	Terra::modelContainer->ReleaseUploadBuffers();
	Terra::textureStorage->ReleaseUploadBuffers();
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
