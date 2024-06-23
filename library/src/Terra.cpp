#ifdef TERRA_WIN32
#include <DisplayManagerWin32.hpp>
#include <SurfaceManagerWin32.hpp>
#include <Exception.hpp>
#else
#include <DisplayManagerVK.hpp>
#endif

#include <Terra.hpp>
#include <RenderEngineVertexShader.hpp>
#include <RenderEngineMeshShader.hpp>

// Queue
Terra::Queue::Queue() : m_queue{ nullptr }, m_cmdBuffer{ nullptr } {}

void InitGraphicsQueue(
	VkQueue vkQueue, VkDevice logicalDevice, std::uint32_t queueIndex,
	std::uint32_t bufferCount, ObjectManager& om, Terra::Queue& queue
)
{
	//om.CreateObject(queue.m_queue, 1u, vkQueue);
	//om.CreateObject(queue.m_cmdBuffer, 1u, logicalDevice, queueIndex, bufferCount);
	//om.CreateObject(queue.m_syncObjects, 1u, logicalDevice, bufferCount, true);
}

void InitTransferQueue(
	VkQueue vkQueue, VkDevice logicalDevice, std::uint32_t queueIndex,
	ObjectManager& om, Terra::Queue& queue
)
{
	//om.CreateObject(queue.m_queue, 1u, vkQueue);
	//om.CreateObject(queue.m_cmdBuffer, 1u, logicalDevice, queueIndex);
	//om.CreateObject(queue.m_syncObjects, 1u, logicalDevice);
}

// Terra Instance
static std::unique_ptr<Terra> sTerra;

Terra::Terra(Terra&& other) noexcept
	: m_appName{ std::move(other.m_appName) }, m_objectManager{ std::move(other.m_objectManager) },
	m_display{ std::move(other.m_display) }, m_vkInstance{ std::move(other.m_vkInstance) }
	, m_surface{ std::move(other.m_surface) }, m_device{ std::move(other.m_device) }
	, m_graphicsQueue{ std::move(other.m_graphicsQueue) }
	, m_computeQueue{ std::move(other.m_computeQueue) }
	, m_transferQueue{ std::move(other.m_transferQueue) }, m_swapChain{ std::move(other.m_swapChain) }
	, m_cameraManager{ std::move(other.m_cameraManager) }
{}

Terra& Terra::operator=(Terra&& other) noexcept
{
	m_appName               = std::move(other.m_appName);
	m_objectManager         = std::move(other.m_objectManager);
	m_display               = std::move(other.m_display);
	m_vkInstance            = std::move(other.m_vkInstance);
	m_surface               = std::move(other.m_surface);
	m_device                = std::move(other.m_device);
	m_graphicsQueue         = std::move(other.m_graphicsQueue);
	m_computeQueue          = std::move(other.m_computeQueue);
	m_transferQueue         = std::move(other.m_transferQueue);
	m_swapChain             = std::move(other.m_swapChain);
	m_cameraManager         = std::move(other.m_cameraManager);

	return *this;
}

Terra::Terra(
	std::string_view appName, void* windowHandle, void* moduleHandle, std::uint32_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	ThreadPool& threadPool,
	RenderEngineType engineType
) : m_appName{ std::move(appName) }, m_objectManager{}, m_display{ nullptr }, m_vkInstance{ nullptr }
	, m_surface{ nullptr }, m_device{ nullptr }
	//, m_graphicsQueue{}, m_computeQueue{}, m_transferQueue{}
	, m_swapChain{ nullptr }
	, m_cameraManager{ nullptr }
{
	InitDisplay();
	InitSurface();

	m_objectManager.CreateObject(m_vkInstance, 5u, m_appName);

	const CoreVersion coreVersion = CoreVersion::V1_3;

	// Add Instance extensions.
	{
		VkInstanceExtensionManager& extensionManager = Instance().ExtensionManager();

		extensionManager.AddExtensions(Display().GetRequiredExtensions());
		extensionManager.AddExtensions(Surface().GetRequiredExtensions());
	}

#ifdef _DEBUG
	Instance().DebugLayers().AddDebugCallback(DebugCallbackType::FileOut);
#endif

	Instance().CreateInstance(coreVersion);
	VkInstance vkInstance = Instance().GetVKInstance();

	Surface().Create(vkInstance, windowHandle, moduleHandle);

	m_objectManager.CreateObject(m_device, 3u);

	const bool meshShader = engineType == RenderEngineType::MeshDraw;

	// Add Device extensions.
	{
		VkDeviceExtensionManager& extensionManager = Device().ExtensionManager();

		extensionManager.AddExtensions(SwapchainManager::GetRequiredExtensions());
		extensionManager.AddExtensions(MemoryManager::GetRequiredExtensions());
		// Need to add DescriptorBuffer

		/*if (meshShader)
			extensionManager.AddExtensions(GraphicsPipelineMeshShader::GetRequiredExtensions());*/
	}
	Device()
		.SetDeviceFeatures(coreVersion)
		.SetPhysicalDeviceAutomatic(vkInstance)
		.CreateLogicalDevice();

	const VkDevice logicalDevice              = Device().GetLogicalDevice();
	const VkPhysicalDevice physicalDevice     = Device().GetPhysicalDevice();
	const VkQueueFamilyMananger& queFamilyMan = Device().GetQueueFamilyManager();

	InitQueues(logicalDevice, bufferCount, queFamilyMan);

	/*Swapchain().CreateSwapchain(
		logicalDevice, physicalDevice, nullptr memoryManager , Surface()
	);*/

	//InitRenderEngine(logicalDevice, engineType, bufferCount, queFamilyMan.GetAllIndices());
}

Terra& Terra::Get() { return *sTerra; }

void Terra::Init(
	std::string_view appName, void* windowHandle, void* moduleHandle, std::uint32_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	ThreadPool& threadPool,
	RenderEngineType engineType
)
{
	sTerra = std::make_unique<Terra>(
		appName, windowHandle, moduleHandle, bufferCount, width, height, threadPool,
		engineType
	);
}

void Terra::InitDisplay()
{
#ifdef TERRA_WIN32
		m_objectManager.CreateObject<IDisplayManager, DisplayManagerWin32>(m_display, 3u);
#else
		m_objectManager.CreateObject<IDisplayManager, DisplayManagerVK>(m_display, 3u);
#endif
}

void Terra::InitSurface()
{
#ifdef TERRA_WIN32
		m_objectManager.CreateObject<SurfaceManager, SurfaceManagerWin32>(m_surface, 3u);
#endif
}

void Terra::InitQueues(
	VkDevice device, std::uint32_t bufferCount, const VkQueueFamilyMananger& queFamily
)
{
	using enum QueueType;

	InitGraphicsQueue(
		queFamily.GetQueue(GraphicsQueue), device, queFamily.GetIndex(GraphicsQueue), bufferCount,
		m_objectManager, m_graphicsQueue
	);
	InitGraphicsQueue(
		queFamily.GetQueue(ComputeQueue), device, queFamily.GetIndex(ComputeQueue), bufferCount,
		m_objectManager, m_computeQueue
	);
	InitTransferQueue(
		queFamily.GetQueue(TransferQueue), device, queFamily.GetIndex(TransferQueue), m_objectManager,
		m_transferQueue
	);
}
