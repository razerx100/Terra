#ifdef TERRA_WIN32
#include <DisplayManagerWin32.hpp>
#include <SurfaceManagerWin32.hpp>
#else
#include <DisplayManagerVK.hpp>
#endif

#include <Terra.hpp>
#include <RenderEngineVS.hpp>
#include <RenderEngineMS.hpp>

Terra::Terra(
	std::string_view appName,
	void* windowHandle, void* moduleHandle, std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount, std::shared_ptr<ThreadPool> threadPool, RenderEngineType engineType
) : m_instanceManager{ std::move(appName) }, m_surfaceManager{ nullptr },
	m_deviceManager{}, m_displayManager{ nullptr }, m_swapchain { nullptr }, m_renderEngine{ nullptr },
	// The width and height are zero initialised, as they will be set with the call to Resize.
	m_windowWidth{ 0u }, m_windowHeight{ 0u }
{
	CreateInstance();

	CreateSurface(windowHandle, moduleHandle);

	CreateDevice(engineType);

	CreateDisplayManager();

	CreateSwapchain(bufferCount);

	CreateRenderEngine(engineType, std::move(threadPool), bufferCount);

	// Need to create the swapchain and frame buffers and stuffs.
	Resize(width, height);
}

void Terra::FinaliseInitialisation()
{
	m_renderEngine->FinaliseInitialisation();
}

void Terra::CreateInstance()
{
#if _DEBUG
	m_instanceManager.DebugLayers().AddDebugCallback(DebugCallbackType::FileOut);
#endif

	{
		VkInstanceExtensionManager& extensionManager = m_instanceManager.ExtensionManager();

#ifdef TERRA_WIN32
		SurfaceInstanceExtensionWin32{}.SetInstanceExtensions(extensionManager);
		DisplayInstanceExtensionWin32{}.SetInstanceExtensions(extensionManager);
#endif
	}

	m_instanceManager.CreateInstance(s_coreVersion);
}

void Terra::CreateSurface(void* windowHandle, void* moduleHandle)
{
#ifdef TERRA_WIN32
	m_surfaceManager = std::make_unique<SurfaceManagerWin32>();
#endif

	m_surfaceManager->Create(m_instanceManager.GetVKInstance(), windowHandle, moduleHandle);
}

void Terra::CreateDevice(RenderEngineType engineType)
{
	{
		VkDeviceExtensionManager& extensionManager = m_deviceManager.ExtensionManager();

		extensionManager.AddExtensions(SwapchainManager::GetRequiredExtensions());

		if (engineType == RenderEngineType::IndividualDraw)
			RenderEngineVSIndividualDeviceExtension{}.SetDeviceExtensions(extensionManager);
		else if (engineType == RenderEngineType::IndirectDraw)
			RenderEngineVSIndirectDeviceExtension{}.SetDeviceExtensions(extensionManager);
		else if (engineType == RenderEngineType::MeshDraw)
			RenderEngineMSDeviceExtension{}.SetDeviceExtensions(extensionManager);
	}

	m_deviceManager.SetDeviceFeatures(s_coreVersion)
		.SetPhysicalDeviceAutomatic(m_instanceManager.GetVKInstance(), *m_surfaceManager)
		.CreateLogicalDevice();
}

void Terra::CreateDisplayManager()
{
#ifdef TERRA_WIN32
	m_displayManager = std::make_unique<DisplayManagerWin32>();
#endif
}

void Terra::CreateSwapchain(std::uint32_t frameCount)
{
	VkDevice device = m_deviceManager.GetLogicalDevice();

	m_swapchain = std::make_unique<SwapchainManager>(
		device, m_deviceManager.GetQueueFamilyManager().GetQueue(QueueType::GraphicsQueue), frameCount
	);
}

void Terra::CreateRenderEngine(
	RenderEngineType engineType, std::shared_ptr<ThreadPool>&& threadPool, std::uint32_t frameCount
) {
	if (engineType == RenderEngineType::IndividualDraw)
		m_renderEngine = std::make_unique<RenderEngineVSIndividual>(
			m_deviceManager, std::move(threadPool), frameCount
		);
	else if (engineType == RenderEngineType::IndirectDraw)
		m_renderEngine = std::make_unique<RenderEngineVSIndirect>(
			m_deviceManager, std::move(threadPool), frameCount
		);
	else if (engineType == RenderEngineType::MeshDraw)
		m_renderEngine = std::make_unique<RenderEngineMS>(
			m_deviceManager, std::move(threadPool), frameCount
		);
}

void Terra::Resize(std::uint32_t width, std::uint32_t height)
{
	// Only recreate these if the new resolution is different.
	if (m_windowWidth != width || m_windowHeight != height)
	{
		WaitForGPUToFinish();

		// Must recreate the swapchain first.
		m_swapchain->CreateSwapchain(
			m_deviceManager.GetPhysicalDevice(), *m_surfaceManager, width, height
		);

		const VkExtent2D newExtent = m_swapchain->GetCurrentSwapchainExtent();

		// Using the new extent, as it might be slightly different than the values we got.
		// For example, the title area of a window can't be drawn upon.
		// So, the actual rendering area would be a bit smaller.
		m_renderEngine->Resize(
			newExtent.width, newExtent.height, m_swapchain->HasSwapchainFormatChanged()
		);

		m_windowWidth  = width;
		m_windowHeight = height;
	}
}

void Terra::Render()
{
	// Semaphores will be initialised as 0. So, the starting value should be 1.
	static std::uint64_t semaphoreCounter = 1u;

	VkDevice device = m_deviceManager.GetLogicalDevice();

	// This semaphore will be signaled when the image becomes available.
	m_swapchain->QueryNextImageIndex(device);

	const VKSemaphore& nextImageSemaphore = m_swapchain->GetNextImageSemaphore();
	const std::uint32_t nextImageIndexU32 = m_swapchain->GetNextImageIndex();
	const size_t nextImageIndex           = nextImageIndexU32;

	VkSemaphore renderFinishedSemaphore = m_renderEngine->Render(
		nextImageIndex, m_swapchain->GetColourAttachment(nextImageIndex),
		m_swapchain->GetCurrentSwapchainExtent(), semaphoreCounter, nextImageSemaphore
	);

	m_swapchain->Present(nextImageIndexU32, renderFinishedSemaphore);
}

void Terra::WaitForGPUToFinish()
{
	vkDeviceWaitIdle(m_deviceManager.GetLogicalDevice());
}

DisplayManager::Resolution Terra::GetFirstDisplayCoordinates() const
{
	return m_displayManager->GetDisplayResolution(m_deviceManager.GetPhysicalDevice(), 0u);
}

VkExtent2D Terra::GetCurrentRenderArea() const noexcept
{
	return m_swapchain->GetCurrentSwapchainExtent();
}

size_t Terra::AddTextureAsCombined(STexture&& texture)
{
	WaitForGPUToFinish();

	return m_renderEngine->AddTextureAsCombined(std::move(texture));
}

std::uint32_t Terra::BindCombinedTexture(size_t textureIndex)
{
	WaitForGPUToFinish();

	return m_renderEngine->BindCombinedTexture(textureIndex);
}

void Terra::RemoveTexture(size_t textureIndex)
{
	WaitForGPUToFinish();

	m_renderEngine->RemoveTexture(textureIndex);
}
