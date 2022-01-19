#include <GraphicsEngineVK.hpp>
#include <InstanceManager.hpp>
#include <CommonPipelineObjects.hpp>

GraphicsEngineVK::GraphicsEngineVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	size_t bufferCount
) : m_backgroundColor{ 0.1f, 0.1f, 0.1f, 0.1f }, m_appName(appName) {

	SetScissorAndViewport(width, height);

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

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = deviceManagerRef->GetQueue(
		QueueType::GraphicsQueue
	);
	GfxQueInst::Init(
		graphicsQueueHandle
	);

	VkDevice logicalDevice = deviceManagerRef->GetLogicalDevice();

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

	SyncObjInst::Init(
		logicalDevice,
		bufferCount
	);

	DisplayInst::GetRef()->InitDisplayManager(
		deviceManagerRef->GetPhysicalDevice()
	);
}

GraphicsEngineVK::~GraphicsEngineVK() noexcept {
	GfxPoolInst::CleanUp();
	SwapChainInst::CleanUp();
	GfxQueInst::CleanUp();
	SyncObjInst::CleanUp();
	DisplayInst::CleanUp();
	DeviceInst::CleanUp();
	SurfaceInst::CleanUp();
#ifdef _DEBUG
	DebugLayerInst::CleanUp();
#endif
	VkInstInst::CleanUp();
}

void GraphicsEngineVK::SetBackgroundColor(const Ceres::VectorF32& colorVector) noexcept {
	m_backgroundColor = {
		colorVector.F32.x, colorVector.F32.y, colorVector.F32.z, colorVector.F32.w
	};
}

void GraphicsEngineVK::SubmitModel(const IModel* const modelRef, bool texture) {

}

void GraphicsEngineVK::Render() {
	ISyncObjects* syncObjectsRef = SyncObjInst::GetRef();
	ISwapChainManager* swapchainRef = SwapChainInst::GetRef();
	ICommandPoolManager* commandPoolRef = GfxPoolInst::GetRef();
	IGraphicsQueueManager* graphicsQueueRef = GfxQueInst::GetRef();

	syncObjectsRef->WaitAndResetFence();
	size_t imageIndex = swapchainRef->GetAvailableImageIndex();
	commandPoolRef->Reset(imageIndex);

	VkCommandBuffer commandBuffer = commandPoolRef->GetCommandBuffer(imageIndex);

	vkCmdSetViewport(commandBuffer, 0u, 1u, &m_viewport);
	vkCmdSetScissor(commandBuffer, 0u, 1u, &m_scissorRect);

	VkImageMemoryBarrier transferBarrier;
	swapchainRef->GetUndefinedToTransferBarrier(imageIndex, transferBarrier);
	vkCmdPipelineBarrier(
		commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0u, 0u, nullptr, 0u, nullptr, 1u, &transferBarrier
	);

	static VkImageSubresourceRange subResourceRange = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		0u, 1u, 0u, 1u
	};

	vkCmdClearColorImage(
		commandBuffer, swapchainRef->GetImage(imageIndex),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &m_backgroundColor,
		1u, &subResourceRange
	);

	VkImageMemoryBarrier presentBarrier;
	swapchainRef->GetTransferToPresentBarrier(imageIndex, presentBarrier);
	vkCmdPipelineBarrier(
		commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0u, 0u, nullptr, 0u, nullptr, 1u, &presentBarrier
	);

	commandPoolRef->Close(imageIndex);

	graphicsQueueRef->SubmitCommandBuffer(commandBuffer);
	swapchainRef->PresentImage(imageIndex);

	syncObjectsRef->ChangeFrameIndex();
}

void GraphicsEngineVK::Resize(std::uint32_t width, std::uint32_t height) {
	bool hasSwapFormatChanged = false;
	SwapChainInst::GetRef()->ResizeSwapchain(width, height, hasSwapFormatChanged);
	// If swapFormatChanged Recreate RenderPass
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

void GraphicsEngineVK::SetScissorAndViewport(
	std::uint32_t width, std::uint32_t height
) noexcept {
	PopulateViewport(m_viewport, width, height);
	PopulateScissorRect(m_scissorRect, width, height);
}

void GraphicsEngineVK::SetShaderPath(const char* path) noexcept {
	m_shaderPath = path;
}

void GraphicsEngineVK::InitResourceBasedObjects() {

}

void GraphicsEngineVK::ProcessData() {

}
