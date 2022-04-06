#include <GraphicsEngineVK.hpp>
#include <InstanceManager.hpp>

GraphicsEngineVK::GraphicsEngineVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	size_t bufferCount
) : m_backgroundColour{}, m_appName(appName),
	m_bufferCount(bufferCount), m_renderPassInfo{} {

	m_backgroundColour.color = {
		0.1f, 0.1f, 0.1f, 0.1f
	};

	m_renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	m_renderPassInfo.renderArea.offset = { 0, 0 };
	m_renderPassInfo.clearValueCount = 1u;

	ViewPAndScsrInst::Init(width, height);

	DisplayInst::Init();

	VkInstInst::Init(appName);

#ifdef _DEBUG
	DebugLayerInst::Init(
		VkInstInst::GetRef()->GetVKInstance()
	);
#endif

#ifdef TERRA_WIN32
	SurfaceInst::InitWin32(
		VkInstInst::GetRef()->GetVKInstance(), windowHandle, moduleHandle
	);
#endif

	DeviceInst::Init();
	IDeviceManager* deviceManagerRef = DeviceInst::GetRef();
	deviceManagerRef->CreatePhysicalDevice(
		VkInstInst::GetRef()->GetVKInstance(),
		SurfaceInst::GetRef()->GetSurface()
	);
	deviceManagerRef->CreateLogicalDevice();

	VkDevice logicalDevice = deviceManagerRef->GetLogicalDevice();

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = deviceManagerRef->GetQueue(
		QueueType::GraphicsQueue
	);
	GfxQueInst::Init(
		logicalDevice,
		graphicsQueueHandle,
		bufferCount
	);

	auto [presentQueueHandle, presentQueueFamilyIndex] = deviceManagerRef->GetQueue(
		QueueType::PresentQueue
	);

	SwapChainInst::Init(
		logicalDevice,
		deviceManagerRef->GetSwapChainInfo(),
		SurfaceInst::GetRef()->GetSurface(),
		width, height, bufferCount,
		presentQueueHandle,
		presentQueueFamilyIndex
	);

	GfxPoolInst::Init(
		logicalDevice,
		graphicsQueueFamilyIndex,
		bufferCount
	);

	auto [copyQueueHandle, copyQueueFamilyIndex] = deviceManagerRef->GetQueue(
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

	CpyQueInst::Init(logicalDevice, copyQueueHandle);

	CpyPoolInst::Init(logicalDevice, copyQueueFamilyIndex);

	VkPhysicalDevice physicalDevice = deviceManagerRef->GetPhysicalDevice();

	DisplayInst::GetRef()->InitDisplayManager(
		physicalDevice
	);

	ISwapChainManager* swapRef = SwapChainInst::GetRef();

	RndrPassInst::Init(logicalDevice, swapRef->GetSwapFormat());

	swapRef->CreateFramebuffers(
		logicalDevice, RndrPassInst::GetRef()->GetRenderPass(),
		width, height
	);

	DescSetMan::Init(logicalDevice);

	VertexBufferInst::Init(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	IndexBufferInst::Init(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
	UniformBufferInst::Init(logicalDevice, physicalDevice, copyAndGfxFamilyIndices);
}

GraphicsEngineVK::~GraphicsEngineVK() noexcept {
	ViewPAndScsrInst::CleanUp();
	ModelContainerInst::CleanUp();
	DescSetMan::CleanUp();
	UniformBufferInst::CleanUp();
	VertexBufferInst::CleanUp();
	IndexBufferInst::CleanUp();
	CpyQueInst::CleanUp();
	CpyPoolInst::CleanUp();
	RndrPassInst::CleanUp();
	SwapChainInst::CleanUp();
	GfxQueInst::CleanUp();
	GfxPoolInst::CleanUp();
	DisplayInst::CleanUp();
	DeviceInst::CleanUp();
	SurfaceInst::CleanUp();
#ifdef _DEBUG
	DebugLayerInst::CleanUp();
#endif
	VkInstInst::CleanUp();
}

void GraphicsEngineVK::SetBackgroundColour(const Ceres::Float32_4& colourVector) noexcept {
	m_backgroundColour.color = {
		colourVector.x, colourVector.y, colourVector.z, colourVector.w
	};
}

void GraphicsEngineVK::SubmitModel(const IModel* const modelRef) {
	ModelContainerInst::GetRef()->AddModel(
		DeviceInst::GetRef()->GetLogicalDevice(), modelRef
	);
}

void GraphicsEngineVK::Render() {
	ISwapChainManager* swapchainRef = SwapChainInst::GetRef();
	ICommandPoolManager* commandPoolRef = GfxPoolInst::GetRef();
	IGraphicsQueueManager* graphicsQueueRef = GfxQueInst::GetRef();

	graphicsQueueRef->WaitForGPU();
	size_t imageIndex = swapchainRef->GetAvailableImageIndex();
	commandPoolRef->Reset(imageIndex);

	VkCommandBuffer commandBuffer = commandPoolRef->GetCommandBuffer(imageIndex);

	IViewportAndScissorManager* viewportRef = ViewPAndScsrInst::GetRef();

	vkCmdSetViewport(commandBuffer, 0u, 1u, viewportRef->GetViewportRef());
	vkCmdSetScissor(commandBuffer, 0u, 1u, viewportRef->GetScissorRef());

	m_renderPassInfo.renderPass = RndrPassInst::GetRef()->GetRenderPass();
	m_renderPassInfo.framebuffer = swapchainRef->GetFramebuffer(imageIndex);
	m_renderPassInfo.renderArea.extent = swapchainRef->GetSwapExtent();
	m_renderPassInfo.pClearValues = &m_backgroundColour;

	vkCmdBeginRenderPass(commandBuffer, &m_renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	ModelContainerInst::GetRef()->BindCommands(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	commandPoolRef->Close(imageIndex);

	graphicsQueueRef->SubmitCommandBuffer(commandBuffer, swapchainRef->GetImageSemaphore());
	swapchainRef->PresentImage(
		static_cast<std::uint32_t>(imageIndex), graphicsQueueRef->GetRenderSemaphore()
	);

	size_t nextFrame = (imageIndex + 1u) % m_bufferCount;
	swapchainRef->SetNextFrameIndex(nextFrame);
	graphicsQueueRef->SetNextFrameIndex(nextFrame);
}

void GraphicsEngineVK::Resize(std::uint32_t width, std::uint32_t height) {
	bool hasSwapFormatChanged = false;
	ISwapChainManager* swapRef = SwapChainInst::GetRef();
	IRenderPassManager* rndrPassRef = RndrPassInst::GetRef();

	if (swapRef->ResizeSwapchain(
		width, height, rndrPassRef->GetRenderPass(), hasSwapFormatChanged
	)) {
		ViewPAndScsrInst::GetRef()->Resize(width, height);

		if (hasSwapFormatChanged)
			rndrPassRef->CreateRenderPass(
				DeviceInst::GetRef()->GetLogicalDevice(), swapRef->GetSwapFormat()
			);
	}
}

void GraphicsEngineVK::GetMonitorCoordinates(
	std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
) {
	Ceres::Rect resolution;
	DisplayInst::GetRef()->GetDisplayResolution(
		DeviceInst::GetRef()->GetPhysicalDevice(),
		resolution
	);

	monitorWidth = resolution.right;
	monitorHeight = resolution.bottom;
}

void GraphicsEngineVK::WaitForAsyncTasks() {
	vkDeviceWaitIdle(
		DeviceInst::GetRef()->GetLogicalDevice()
	);
}

void GraphicsEngineVK::SetShaderPath(const char* path) noexcept {
	m_shaderPath = path;
}

void GraphicsEngineVK::InitResourceBasedObjects() {
	ModelContainerInst::Init(m_shaderPath.c_str());
}

void GraphicsEngineVK::ProcessData() {
	VkDevice logicalDevice = DeviceInst::GetRef()->GetLogicalDevice();

	IModelContainer* modelContainerRef = ModelContainerInst::GetRef();
	DescSetMan::GetRef()->CreateDescriptorSets(logicalDevice);
	UniformBufferInst::GetRef()->CreateBuffer(logicalDevice);
	modelContainerRef->CreateBuffers(logicalDevice);
	modelContainerRef->CopyData();

	ICommandPoolManager* copyBufferManager = CpyPoolInst::GetRef();
	copyBufferManager->Reset(0u);
	VkCommandBuffer copyBuffer = copyBufferManager->GetCommandBuffer(0u);

	modelContainerRef->RecordUploadBuffers(logicalDevice, copyBuffer);

	copyBufferManager->Close(0u);

	ICopyQueueManager* copyQueue = CpyQueInst::GetRef();
	copyQueue->SubmitCommandBuffer(copyBuffer);
	copyQueue->WaitForGPU();

	modelContainerRef->InitPipelines(logicalDevice);

	modelContainerRef->ReleaseUploadBuffers(logicalDevice);
}

size_t GraphicsEngineVK::RegisterResource(
	const void* data,
	size_t width, size_t height, size_t pixelSizeInBytes
) {
	UniformBufferInst::GetRef()->AddBuffer(
		DeviceInst::GetRef()->GetLogicalDevice(),
		data, width * height * pixelSizeInBytes
	);
	return 0u;
}
