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
) : m_backgroundColor{ 0.1f, 0.1f, 0.1f, 0.1f }, m_appName(appName) {

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
	if (width == 0 && height == 0)
		vkDeviceWaitIdle(GetDeviceManagerInstance()->GetLogicalDevice());
	else {
		bool hasSwapFormatChanged = false;
		GetSwapchainManagerInstance()->ResizeSwapchain(width, height, hasSwapFormatChanged);
		// If swapFormatChanged Recreate RenderPass
	}
}

SRect GraphicsEngineVK::GetMonitorCoordinates() {
	std::uint32_t displayCount;
	VkPhysicalDevice gpu = GetDeviceManagerInstance()->GetPhysicalDevice();
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, nullptr);

	std::vector<VkDisplayPropertiesKHR> displayProperties(displayCount);
	vkGetPhysicalDeviceDisplayPropertiesKHR(gpu, &displayCount, displayProperties.data());

	SRect displayRect = {};
	if (displayCount == 1) {
		displayRect.left = 0u;
		displayRect.right = displayProperties[0].physicalResolution.width;
		displayRect.top = 0u;
		displayRect.bottom = displayProperties[0].physicalResolution.height;
	}

	return displayRect;
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
