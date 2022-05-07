#include <RendererVK.hpp>
#include <Terra.hpp>

RendererVK::RendererVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount
) : m_backgroundColour{}, m_appName(appName),
	m_bufferCount(bufferCount) {

	m_backgroundColour.color = {
		{0.1f, 0.1f, 0.1f, 0.1f }
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

	auto [presentQueueHandle, presentQueueFamilyIndex] = Terra::device->GetQueue(
		QueueType::PresentQueue
	);

	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	SwapChainManagerCreateInfo swapCreateInfo = {};
	swapCreateInfo.device = logicalDevice;
	swapCreateInfo.surfaceInfo = QuerySurfaceCapabilities(physicalDevice, vkSurface);
	swapCreateInfo.surface = vkSurface;
	swapCreateInfo.width = width;
	swapCreateInfo.height = height;
	swapCreateInfo.bufferCount = bufferCount;

	Terra::InitSwapChain(swapCreateInfo, presentQueueHandle, presentQueueFamilyIndex);

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

	Terra::InitRenderPass(logicalDevice, Terra::swapChain->GetSwapFormat());

	Terra::swapChain->CreateFramebuffers(
		logicalDevice, Terra::renderPass->GetRenderPass(),
		width, height
	);

	Terra::InitDescriptorSet(logicalDevice);

	Terra::InitVertexBuffer(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	Terra::InitIndexBuffer(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	Terra::InitUniformBuffer(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	Terra::InitTextureStorage(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
}

RendererVK::~RendererVK() noexcept {
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
	m_backgroundColour.color = {
		{colourVector[0], colourVector[1], colourVector[2], colourVector[3]}
	};
}

void RendererVK::SubmitModel(const IModel* const modelRef) {
	Terra::modelContainer->AddModel(
		Terra::device->GetLogicalDevice(), modelRef
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

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.clearValueCount = 1u;
	renderPassInfo.renderPass = Terra::renderPass->GetRenderPass();
	renderPassInfo.framebuffer = Terra::swapChain->GetFramebuffer(imageIndex);
	renderPassInfo.renderArea.extent = Terra::swapChain->GetSwapExtent();
	renderPassInfo.pClearValues = &m_backgroundColour;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	Terra::modelContainer->BindCommands(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	Terra::graphicsCmdPool->Close(imageIndex);

	Terra::graphicsQueue->SubmitCommandBuffer(
		commandBuffer, Terra::swapChain->GetImageSemaphore()
	);
	Terra::swapChain->PresentImage(
		static_cast<std::uint32_t>(imageIndex), Terra::graphicsQueue->GetRenderSemaphore()
	);

	const size_t nextFrame =
		++imageIndex >= m_bufferCount ? imageIndex % m_bufferCount : imageIndex;

	Terra::swapChain->SetNextFrameIndex(nextFrame);
	Terra::graphicsQueue->SetNextFrameIndex(nextFrame);
}

void RendererVK::Resize(std::uint32_t width, std::uint32_t height) {
	bool hasSwapFormatChanged = false;
	if (Terra::swapChain->ResizeSwapchain(
		Terra::device->GetLogicalDevice(),
		Terra::surface->GetSurface(),
		width, height, Terra::renderPass->GetRenderPass(),
		hasSwapFormatChanged
	)) {
		Terra::viewportAndScissor->Resize(width, height);

		if (hasSwapFormatChanged)
			Terra::renderPass->CreateRenderPass(
				Terra::device->GetLogicalDevice(), Terra::swapChain->GetSwapFormat()
			);
	}
}

Renderer::Resolution RendererVK::GetDisplayCoordinates(std::uint32_t displayIndex) const {
	return Terra::display->GetDisplayResolution(
		Terra::device->GetPhysicalDevice(), displayIndex
	);
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
	Terra::InitModelContainer(m_shaderPath);
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
	const void* data,
	size_t width, size_t height, size_t pixelSizeInBytes
) {
	return Terra::textureStorage->AddTexture(
		Terra::device->GetLogicalDevice(),
		data, width, height, pixelSizeInBytes
	);
}

void RendererVK::SetThreadPool(std::shared_ptr<IThreadPool> threadPoolArg) noexcept {
	Terra::SetThreadPool(std::move(threadPoolArg));
}

