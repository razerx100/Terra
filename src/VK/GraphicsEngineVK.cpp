#include <GraphicsEngineVK.hpp>
#include <IInstanceManager.hpp>
#include <ISurfaceManager.hpp>
#include <IDeviceManager.hpp>
#include <IGraphicsQueueManager.hpp>
#include <ISwapChainManager.hpp>
#include <ICommandPoolManager.hpp>
#include <SyncObjects.hpp>
#include <CommonPipelineObjects.hpp>

GraphicsEngineVK::GraphicsEngineVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) : m_backgroundColor{ 0.1f, 0.1f, 0.1f, 0.1f }, m_appName(appName), m_subResourceRange{} {

	SetScissorAndViewport(width, height);

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

	m_subResourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	m_subResourceRange.baseMipLevel = 0;
	m_subResourceRange.levelCount = 1u;
	m_subResourceRange.baseArrayLayer = 0;
	m_subResourceRange.layerCount = 1u;

}

GraphicsEngineVK::~GraphicsEngineVK() noexcept {
	CleanUpGraphicsPoolManagerInstance();
	CleanUpSwapchainManagerInstance();
	CleanUpGraphicsQueueManagerInstance();
	CleanUpSyncObjectsInstance();
	CleanUpDeviceManagerInstance();
	CleanUpSurfaceManagerInstance();
	CleanUpInstanceManagerInstance();
}

void GraphicsEngineVK::SetBackgroundColor(Color color) noexcept {
	m_backgroundColor = { {color.r, color.g, color.b, color.a} };
}

void GraphicsEngineVK::SubmitCommands() {

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

	VkImageMemoryBarrier renderBarrier = swapchainRef->GetPresentToRenderBarrier(imageIndex);
	vkCmdPipelineBarrier(
		commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1u, &renderBarrier
	);

	vkCmdClearColorImage(
		commandBuffer, swapchainRef->GetImage(imageIndex),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &m_backgroundColor,
		1u, &m_subResourceRange
	);

	VkImageMemoryBarrier presentBarrier = swapchainRef->GetRenderToPresentBarrier(imageIndex);
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

}

SRect GraphicsEngineVK::GetMonitorCoordinates() {
	return SRect{};
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
