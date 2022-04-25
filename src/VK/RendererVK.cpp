#include <RendererVK.hpp>
#include <Terra.hpp>

RendererVK::RendererVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount
) : m_backgroundColour{}, m_appName(appName),
	m_bufferCount(bufferCount), m_renderPassInfo{} {

	m_backgroundColour.color = {
		{0.1f, 0.1f, 0.1f, 0.1f }
	};

	m_renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	m_renderPassInfo.renderArea.offset = { 0, 0 };
	m_renderPassInfo.clearValueCount = 1u;

	Terra::InitViewportAndScissor(width, height);

	Terra::InitDisplay();

	Terra::InitVkInstance(appName);

	VkInstance vkInstance = Terra::vkInstance->GetVKInstance();

#ifdef _DEBUG
	Terra::InitDebugLayer(vkInstance);
#endif

#ifdef TERRA_WIN32
	Terra::InitSurface(vkInstance, windowHandle, moduleHandle);
#endif

	Terra::InitDevice();

	VkSurfaceKHR vkSurface = Terra::surface->GetSurface();

	Terra::device->CreatePhysicalDevice(
		vkInstance,
		vkSurface
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

	SwapChainManagerCreateInfo swapCreateInfo = {};
	swapCreateInfo.device = logicalDevice;
	swapCreateInfo.capabilities = Terra::device->GetSwapChainInfo();
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

	VkPhysicalDevice physicalDevice = Terra::device->GetPhysicalDevice();

	Terra::display->InitDisplayManager(
		physicalDevice
	);

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

void RendererVK::SetBackgroundColour(const Ceres::Float32_4& colourVector) noexcept {
	m_backgroundColour.color = {
		{colourVector.x, colourVector.y, colourVector.z, colourVector.w}
	};
}

void RendererVK::SubmitModel(const IModel* const modelRef) {
	Terra::modelContainer->AddModel(
		Terra::device->GetLogicalDevice(), modelRef
	);
}

void RendererVK::Render() {
	Terra::graphicsQueue->WaitForGPU();
	const size_t imageIndex = Terra::swapChain->GetAvailableImageIndex();
	Terra::graphicsCmdPool->Reset(imageIndex);

	VkCommandBuffer commandBuffer = Terra::graphicsCmdPool->GetCommandBuffer(imageIndex);

	vkCmdSetViewport(commandBuffer, 0u, 1u, Terra::viewportAndScissor->GetViewportRef());
	vkCmdSetScissor(commandBuffer, 0u, 1u, Terra::viewportAndScissor->GetScissorRef());

	m_renderPassInfo.renderPass = Terra::renderPass->GetRenderPass();
	m_renderPassInfo.framebuffer = Terra::swapChain->GetFramebuffer(imageIndex);
	m_renderPassInfo.renderArea.extent = Terra::swapChain->GetSwapExtent();
	m_renderPassInfo.pClearValues = &m_backgroundColour;

	vkCmdBeginRenderPass(commandBuffer, &m_renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	Terra::modelContainer->BindCommands(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	Terra::graphicsCmdPool->Close(imageIndex);

	Terra::graphicsQueue->SubmitCommandBuffer(
		commandBuffer, Terra::swapChain->GetImageSemaphore()
	);
	Terra::swapChain->PresentImage(
		static_cast<std::uint32_t>(imageIndex), Terra::graphicsQueue->GetRenderSemaphore()
	);

	const size_t nextFrame = (imageIndex + 1u) % m_bufferCount;
	Terra::swapChain->SetNextFrameIndex(nextFrame);
	Terra::graphicsQueue->SetNextFrameIndex(nextFrame);
}

void RendererVK::Resize(std::uint32_t width, std::uint32_t height) {
	bool hasSwapFormatChanged = false;
	if (Terra::swapChain->ResizeSwapchain(
		Terra::device->GetLogicalDevice(),
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

void RendererVK::GetMonitorCoordinates(
	std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
) {
	Ceres::Rect resolution = {};
	Terra::display->GetDisplayResolution(
		Terra::device->GetPhysicalDevice(),
		resolution
	);

	monitorWidth = resolution.right;
	monitorHeight = resolution.bottom;
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

	// Transition image layouts
	Terra::graphicsCmdPool->Reset(0u);
	VkCommandBuffer graphicsBuffer = Terra::graphicsCmdPool->GetCommandBuffer(0u);

	Terra::textureStorage->TransitionImages(graphicsBuffer);

	Terra::graphicsCmdPool->Close(0u);

	Terra::graphicsQueue->SubmitCommandBuffer(graphicsBuffer);
	Terra::graphicsQueue->WaitForGPU();

	Terra::textureStorage->SetDescriptorLayouts(logicalDevice);

	Terra::descriptorSet->CreateDescriptorSets(logicalDevice);
	Terra::modelContainer->InitPipelines(logicalDevice);

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

