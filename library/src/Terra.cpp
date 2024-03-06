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

// Resources
Terra::Resources::Resources()
	: m_gpuOnlyMemory{ nullptr }, m_uploadMemory{ nullptr }, m_cpuWriteMemory{ nullptr },
m_uploadContainer{ nullptr }
{}

void Terra::Resources::Init(
	ObjectManager& om, VkPhysicalDevice physicalDevice, VkDevice logicalDevice, IThreadPool& threadPool
)
{
	om.CreateObject(
		m_cpuWriteMemory, 2u, logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	om.CreateObject(
		m_uploadMemory, 2u, logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	om.CreateObject(
		m_gpuOnlyMemory, 2u, logicalDevice, physicalDevice, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
	om.CreateObject(m_uploadContainer, 0u, threadPool);
}

// Queue
Terra::Queue::Queue() : m_queue{ nullptr }, m_cmdBuffer{ nullptr }, m_syncObjects{ nullptr } {}

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
	, m_res{ std::move(other.m_res) }, m_graphicsQueue{ std::move(other.m_graphicsQueue) }
	, m_computeQueue{ std::move(other.m_computeQueue) }
	, m_transferQueue{ std::move(other.m_transferQueue) }, m_swapChain{ std::move(other.m_swapChain) }
	, m_graphicsDescriptorSet{ std::move(other.m_graphicsDescriptorSet) }
	, m_computeDescriptorSet{ std::move(other.m_computeDescriptorSet) }
	, m_renderEngine{ std::move(other.m_renderEngine) }
	, m_textureStorage{ std::move(other.m_textureStorage) }
	, m_bufferManager{ std::move(other.m_bufferManager) }
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
	m_res                   = std::move(other.m_res);
	m_graphicsQueue         = std::move(other.m_graphicsQueue);
	m_computeQueue          = std::move(other.m_computeQueue);
	m_transferQueue         = std::move(other.m_transferQueue);
	m_swapChain             = std::move(other.m_swapChain);
	m_graphicsDescriptorSet = std::move(other.m_graphicsDescriptorSet);
	m_computeDescriptorSet  = std::move(other.m_computeDescriptorSet);
	m_renderEngine          = std::move(other.m_renderEngine);
	m_textureStorage        = std::move(other.m_textureStorage);
	m_bufferManager         = std::move(other.m_bufferManager);
	m_cameraManager         = std::move(other.m_cameraManager);

	return *this;
}

Terra::Terra(
	std::string_view appName, void* windowHandle, void* moduleHandle, std::uint32_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	IThreadPool& threadPool, ISharedDataContainer& sharedContainer,
	RenderEngineType engineType
) : m_appName{ std::move(appName) }, m_objectManager{}, m_display{ nullptr }, m_vkInstance{ nullptr }
	, m_surface{ nullptr }, m_device{ nullptr }, m_res{}
	//, m_graphicsQueue{}, m_computeQueue{}, m_transferQueue{}
	, m_swapChain{ nullptr }
	, m_graphicsDescriptorSet{ nullptr }, m_computeDescriptorSet{ nullptr }, m_renderEngine{ nullptr }
	, m_textureStorage{ nullptr }, m_bufferManager{ nullptr }, m_cameraManager{ nullptr }
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

		if (meshShader)
			extensionManager.AddExtensions(GraphicsPipelineMeshShader::GetRequiredExtensions());
	}
	Device()
		.SetDeviceFeatures(coreVersion)
		.SetPhysicalDeviceAutomatic(vkInstance)
		.CreateLogicalDevice();

	const VkDevice logicalDevice              = Device().GetLogicalDevice();
	const VkPhysicalDevice physicalDevice     = Device().GetPhysicalDevice();
	const VkQueueFamilyMananger& queFamilyMan = Device().GetQueueFamilyManager();

	_vkResourceView::SetBufferAlignments(physicalDevice);

	m_res.Init(m_objectManager, physicalDevice, logicalDevice, threadPool);

	InitQueues(logicalDevice, bufferCount, queFamilyMan);

	m_objectManager.CreateObject(
		m_swapChain, 1u, logicalDevice,
		queFamilyMan.GetQueue(GraphicsQueue), bufferCount, nullptr /* memoryManager */
	);
	Swapchain().CreateSwapchain(
		logicalDevice, physicalDevice, nullptr /* memoryManager */, Surface()
	);

	m_objectManager.CreateObject(m_graphicsDescriptorSet, 1u, logicalDevice, bufferCount);
	m_objectManager.CreateObject(m_computeDescriptorSet, 1u, logicalDevice, bufferCount);

	InitRenderEngine(logicalDevice, engineType, bufferCount, queFamilyMan.GetAllIndices());

	m_objectManager.CreateObject(
		m_textureStorage, 1u,
		logicalDevice, physicalDevice, queFamilyMan.GetTransferAndGraphicsIndices()
	);

	const bool modelDataNoBB = (engineType == RenderEngineType::IndirectDraw ? false : true);

	m_objectManager.CreateObject(
		m_bufferManager, 1u,
		logicalDevice,  bufferCount,queFamilyMan.GetComputeAndGraphicsIndices(), modelDataNoBB,
		meshShader, sharedContainer
	);

	m_objectManager.CreateObject(m_cameraManager, 0u, sharedContainer);
}

Terra& Terra::Get() { return *sTerra; }

void Terra::Init(
	std::string_view appName, void* windowHandle, void* moduleHandle, std::uint32_t bufferCount,
	std::uint32_t width, std::uint32_t height,
	IThreadPool& threadPool, ISharedDataContainer& sharedContainer,
	RenderEngineType engineType
)
{
	sTerra = std::make_unique<Terra>(
		appName, windowHandle, moduleHandle, bufferCount, width, height, threadPool, sharedContainer,
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

void Terra::InitRenderEngine(
	VkDevice device, RenderEngineType engineType, std::uint32_t bufferCount, QueueIndices3 queueIndices
)
{
	switch (engineType)
	{
	case RenderEngineType::IndirectDraw:
	{
		m_objectManager.CreateObject<RenderEngine, RenderEngineIndirectDraw>(
			m_renderEngine, 1u, device, bufferCount, queueIndices
		);
		break;
	}
	case RenderEngineType::MeshDraw:
	{
		m_objectManager.CreateObject<RenderEngine, RenderEngineMeshShader>(
			m_renderEngine, 1u, device, bufferCount, queueIndices
		);
		break;
	}
	case RenderEngineType::IndividualDraw:
	default:
	{
		m_objectManager.CreateObject<RenderEngine, RenderEngineIndividualDraw>(
			m_renderEngine, 1u, device, queueIndices
		);
		break;
	}
	}
}
