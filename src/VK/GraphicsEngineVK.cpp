#include <GraphicsEngineVK.hpp>
#include <IInstanceManager.hpp>
#include <ISurfaceManager.hpp>
#include <IDeviceManager.hpp>
#include <IGraphicsQueueManager.hpp>
#include <ISwapChainManager.hpp>
#include <ICommandPoolManager.hpp>
#include <SyncObjects.hpp>
#include <CommonPipelineObjects.hpp>
#include <IDisplayManager.hpp>
#include <CRSStructures.hpp>

GraphicsEngineVK::GraphicsEngineVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) : m_backgroundColor{ 0.1f, 0.1f, 0.1f, 0.1f }, m_appName(appName) {

	SetScissorAndViewport(width, height);

	InitDisplayManagerInstance();

	InitInstanceManagerInstance(appName);

	InitSurfaceManagerInstance(
		GetInstanceManagerInstance()->GetVKInstance(), windowHandle, moduleHandle
	);

	InitDeviceManagerInstance();
	IDeviceManager* deviceManagerRef = GetDeviceManagerInstance();
	deviceManagerRef->CreatePhysicalDevice(
		GetInstanceManagerInstance()->GetVKInstance(),
		GetSurfaceManagerInstance()->GetSurface()
	);
	deviceManagerRef->CreateLogicalDevice();

	auto [graphicsQueueHandle, graphicsQueueFamilyIndex] = deviceManagerRef->GetQueue(
		QueueType::GraphicsQueue
	);
	InitGraphicsQueueManagerInstance(
		graphicsQueueHandle
	);

	VkDevice logicalDevice = deviceManagerRef->GetLogicalDevice();

	auto [presentQueueHandle, presentQueueFamilyIndex] = deviceManagerRef->GetQueue(
		QueueType::PresentQueue
	);

	InitSwapchainManagerInstance(
		logicalDevice,
		deviceManagerRef->GetSwapChainInfo(),
		GetSurfaceManagerInstance()->GetSurface(),
		width, height, bufferCount,
		presentQueueHandle,
		presentQueueFamilyIndex
	);

	InitGraphicsPoolManagerInstance(
		logicalDevice,
		graphicsQueueFamilyIndex,
		bufferCount
	);

	InitSyncObjectsInstance(
		logicalDevice,
		bufferCount
	);

	GetDisplayManagerInstance()->InitDisplayManager(
		deviceManagerRef->GetPhysicalDevice()
	);
}

GraphicsEngineVK::~GraphicsEngineVK() noexcept {
	CleanUpGraphicsPoolManagerInstance();
	CleanUpSwapchainManagerInstance();
	CleanUpGraphicsQueueManagerInstance();
	CleanUpSyncObjectsInstance();
	CleanUpDisplayManagerInstance();
	CleanUpDeviceManagerInstance();
	CleanUpSurfaceManagerInstance();
	CleanUpInstanceManagerInstance();
}

void GraphicsEngineVK::SetBackgroundColor(const float* colorVector) noexcept {
	m_backgroundColor = { colorVector[0], colorVector[1], colorVector[2], colorVector[3] };
}

void GraphicsEngineVK::SubmitModels(const IModel* const models, std::uint32_t modelCount) {

}

void GraphicsEngineVK::Render() {
	ISyncObjects* syncObjectsRef = GetSyncObjectsInstance();
	ISwapChainManager* swapchainRef = GetSwapchainManagerInstance();
	ICommandPoolManager* commandPoolRef = GetGraphicsPoolManagerInstance();
	IGraphicsQueueManager* graphicsQueueRef = GetGraphicsQueueManagerInstance();

	syncObjectsRef->WaitAndResetFence();
	std::uint32_t imageIndex = swapchainRef->GetAvailableImageIndex();
	commandPoolRef->Reset(imageIndex);

	VkCommandBuffer commandBuffer = commandPoolRef->GetCommandBuffer(imageIndex);

	vkCmdSetViewport(commandBuffer, 0u, 1u, &m_viewport);
	vkCmdSetScissor(commandBuffer, 0u, 1u, &m_scissorRect);

	VkImageMemoryBarrier transferBarrier;
	swapchainRef->GetUndefinedToTransferBarrier(imageIndex, transferBarrier);
	vkCmdPipelineBarrier(
		commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1u, &transferBarrier
	);

	static VkImageSubresourceRange subResourceRange = {
		VK_IMAGE_ASPECT_COLOR_BIT,
		0, 1u, 0, 1u
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
		0, 0, nullptr, 0, nullptr, 1u, &presentBarrier
	);

	commandPoolRef->Close(imageIndex);

	graphicsQueueRef->SubmitCommandBuffer(commandBuffer);
	swapchainRef->PresentImage(imageIndex);

	syncObjectsRef->ChangeFrameIndex();
}

void GraphicsEngineVK::Resize(std::uint32_t width, std::uint32_t height) {
	bool hasSwapFormatChanged = false;
	GetSwapchainManagerInstance()->ResizeSwapchain(width, height, hasSwapFormatChanged);
	// If swapFormatChanged Recreate RenderPass
}

void GraphicsEngineVK::GetMonitorCoordinates(std::uint64_t* rectCoord) {
	Ceres::Rect resolution;
	GetDisplayManagerInstance()->GetDisplayResolution(
		GetDeviceManagerInstance()->GetPhysicalDevice(),
		resolution
	);

	std::memcpy(rectCoord, &resolution, 4u);
}

void GraphicsEngineVK::WaitForAsyncTasks() {
	vkDeviceWaitIdle(
		GetDeviceManagerInstance()->GetLogicalDevice()
	);
}

void GraphicsEngineVK::SetScissorAndViewport(
	std::uint32_t width, std::uint32_t height
) noexcept {
	PopulateViewport(m_viewport, width, height);
	PopulateScissorRect(m_scissorRect, width, height);
}
