#ifdef TERRA_WIN32
#include <DisplayManagerWin32.hpp>
#include <SurfaceManagerWin32.hpp>
#else
#include <DisplayManagerVK.hpp>
#endif

#include <Terra.hpp>
#include <RenderEngineVertexShader.hpp>
#include <RenderEngineMeshShader.hpp>

Terra::Terra(
	std::string_view appName,
	void* windowHandle, void* moduleHandle, std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount, std::shared_ptr<ThreadPool> threadPool, RenderEngineType engineType
) : m_instanceManager{ std::move(appName) }, m_surfaceManager{ nullptr },
m_deviceManager{}, m_displayManager{ nullptr }, m_swapchain { nullptr }, m_renderEngine{ nullptr }
{
	CreateInstance();

	CreateSurface(windowHandle, moduleHandle);

	CreateDevice(engineType);

	CreateDisplayManager();

	CreateSwapchain(bufferCount);

	CreateRenderEngine(engineType, std::move(threadPool), bufferCount);

	Resize(width, height);
}

void Terra::CreateInstance()
{
#if _DEBUG
	m_instanceManager.DebugLayers().AddDebugCallback(DebugCallbackType::standardError);
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

	m_swapchain->CreateSwapchain(m_deviceManager, *m_surfaceManager);
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
	m_renderEngine->Resize(
		width, height,
		m_swapchain->HasSwapchainFormatChanged(), m_swapchain->GetSwapchainFormat()
	);

	m_swapchain->CreateFramebuffers(
		m_deviceManager.GetLogicalDevice(), m_renderEngine->GetRenderPass(),
		m_renderEngine->GetDepthBuffer()
	);
}

void Terra::Render()
{
	static size_t nextPotentialImageIndex = 0u;

	{
		VkDevice device = m_deviceManager.GetLogicalDevice();

		const VKSemaphore& waitSemaphore = m_renderEngine->GetGraphicsWait(nextPotentialImageIndex);

		// This is kinda dumb. You need to know the index of the next semaphore to query the next image
		// index. Which might be different. So, can't use that index anywhere else. And have to use
		// the index which is queried from the swapchain.
		m_swapchain->QueryNextImageIndex(device, waitSemaphore);
	}

	const std::uint32_t nextImageIndexU32 = m_swapchain->GetNextImageIndex();
	const size_t nextImageIndex           = nextImageIndexU32;

	m_renderEngine->Render(
		nextImageIndex, m_swapchain->GetFramebuffer(nextImageIndex),
		m_swapchain->GetCurrentSwapchainExtent()
	);

	const VKSemaphore& waitSemaphore = m_renderEngine->GetGraphicsWait(nextImageIndex);

	m_swapchain->Present(nextImageIndexU32, waitSemaphore);

	const size_t frameCount = m_swapchain->GetSwapchain().GetImageCount();
	nextPotentialImageIndex = (nextPotentialImageIndex + 1u) % frameCount;
}
