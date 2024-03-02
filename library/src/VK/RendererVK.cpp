#include <array>

#include <RendererVK.hpp>
#include <Terra.hpp>

RendererVK::RendererVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount,
	IThreadPool& threadPool, ISharedDataContainer& sharedContainer,
	RenderEngineType engineType
) : m_width{ width }, m_height{ height } {

	assert(bufferCount >= 1u && "BufferCount must not be zero.");
	assert(windowHandle && moduleHandle && "Invalid Window or WindowModule Handle.");

	Terra::Init(
		appName, windowHandle, moduleHandle, bufferCount, width, height, threadPool, sharedContainer,
		engineType
	);
	Terra& terra = Terra::Get();

	const VkDevice device = terra.Device().GetLogicalDevice();

	terra.Engine().ResizeViewportAndScissor(width, height);

	terra.Camera().SetSceneResolution(width, height);
}

void RendererVK::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept {
	Terra::Get().Engine().SetBackgroundColour(colourVector);
}

void RendererVK::AddModelSet(
	std::vector<std::shared_ptr<IModel>>&& models, const std::wstring& fragmentShader
) {
	Terra& terra = Terra::Get();

	terra.Engine().RecordModelDataSet(models, fragmentShader + L".spv");
	terra.Buffers().AddOpaqueModels(std::move(models));
}

void RendererVK::AddMeshletModelSet(
	std::vector<MeshletModel>&& meshletModels, const std::wstring& fragmentShader
) {
	Terra& terra = Terra::Get();

	terra.Engine().AddMeshletModelSet(meshletModels, fragmentShader + L".spv");
	terra.Buffers().AddOpaqueModels(std::move(meshletModels));
}

void RendererVK::AddModelInputs(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
) {
	Terra& terra = Terra::Get();

	terra.Engine().AddGVerticesAndIndices(
		terra.Device().GetLogicalDevice(), std::move(gVertices), std::move(gIndices)
	);
}

void RendererVK::AddModelInputs(
	std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
	std::vector<std::uint32_t>&& gPrimIndices
) {
	Terra& terra = Terra::Get();

	terra.Engine().AddGVerticesAndPrimIndices(
		terra.Device().GetLogicalDevice(), std::move(gVertices), std::move(gVerticesIndices),
		std::move(gPrimIndices)
	);
}

void RendererVK::Update()
{
	Terra& terra = Terra::Get();

	terra.Swapchain().QueryNextImageIndex(
		terra.Device().GetLogicalDevice(),
		terra.Graphics().SyncObj().GetFrontSemaphore()
	);
	const size_t imageIndex = terra.Swapchain().GetNextImageIndex();

	terra.Engine().UpdateModelBuffers(static_cast<VkDeviceSize>(imageIndex));
}

void RendererVK::Render() {
	Terra& terra = Terra::Get();

	const size_t imageIndex = terra.Swapchain().GetNextImageIndex();
	const VkCommandBuffer graphicsCommandBuffer = terra.Graphics().CmdBuffer().GetCommandBuffer(
		imageIndex
	);

	RenderEngine& engine = terra.Engine();

	engine.ExecutePreRenderStage(graphicsCommandBuffer, imageIndex);
	engine.RecordDrawCommands(graphicsCommandBuffer, imageIndex);
	engine.Present(graphicsCommandBuffer, imageIndex);
	engine.ExecutePostRenderStage();
}

void RendererVK::Resize(std::uint32_t width, std::uint32_t height) {
	if (width != m_width || height != m_height) {
		m_width = width;
		m_height = height;

		Terra& terra = Terra::Get();

		VkDeviceManager& device = terra.Device();

		vkDeviceWaitIdle(device.GetLogicalDevice());

		RenderEngine& engine = terra.Engine();

		SwapchainManager& swapchain = terra.Swapchain();

		swapchain.CreateSwapchain(
			device.GetLogicalDevice(), device.GetPhysicalDevice(),
			nullptr /* memoryManager */, terra.Surface(),
			width, height
		);

		engine.ResizeViewportAndScissor(width, height);

		terra.Camera().SetSceneResolution(width, height);
	}
}

Renderer::Resolution RendererVK::GetFirstDisplayCoordinates() const {
	Terra& terra = Terra::Get();

	IDisplayManager::Resolution resolution = terra.Display().GetDisplayResolution(
		terra.Device().GetPhysicalDevice(), 0u
	);

	return { resolution.first, resolution.second };
}

void RendererVK::SetShaderPath(const wchar_t* path) noexcept {
	Terra::Get().Engine().SetShaderPath(path);
}

void RendererVK::ProcessData()
{
	Terra& terra = Terra::Get();

	const VkDevice logicalDevice = terra.Device().GetLogicalDevice();

	BufferManager& buffers = terra.Buffers();
	RenderEngine& engine = terra.Engine();

	// Create Buffers
	buffers.CreateBuffers(logicalDevice);
	engine.CreateBuffers(logicalDevice);

	DeviceMemory& gpuMem = terra.Res().GPU();
	DeviceMemory& cpuMem = terra.Res().CPU();
	DeviceMemory& uploadMem = terra.Res().Upload();

	// Allocate Memory
	gpuMem.AllocateMemory(logicalDevice);
	uploadMem.AllocateMemory(logicalDevice);
	cpuMem.AllocateMemory(logicalDevice);

	// Map cpu memories
	uploadMem.MapMemoryToCPU(logicalDevice);
	cpuMem.MapMemoryToCPU(logicalDevice);

	// Set Upload Memory Start
	UploadContainer& uploadContainer = terra.Res().UploadCont();
	uploadContainer.SetMemoryStart(uploadMem.GetMappedCPUPtr());

	// Bind Buffers to memory
	buffers.BindResourceToMemory(logicalDevice);
	engine.BindResourcesToMemory(logicalDevice);
	terra.Texture().BindMemories(logicalDevice);

	// Async Copy
	std::atomic_size_t works = 0u;

	uploadContainer.CopyData(works);
	engine.CopyData();

	while (works != 0u);

	// Upload to GPU
	VKCommandBuffer& transferCmdBuffer = terra.Transfer().CmdBuffer();
	transferCmdBuffer.ResetFirstBuffer();
	const VkCommandBuffer vkTransferCmdBuffer = transferCmdBuffer.GetFirstCommandBuffer();

	TextureStorage& textures = terra.Texture();
	engine.RecordCopy(vkTransferCmdBuffer);
	textures.RecordUploads(vkTransferCmdBuffer);

	textures.ReleaseOwnerships(vkTransferCmdBuffer);
	engine.ReleaseOwnership(vkTransferCmdBuffer);

	transferCmdBuffer.CloseFirstBuffer();

	VkCommandQueue& transferQueue = terra.Transfer().Que();
	VkSyncObjects& transferSync = terra.Transfer().SyncObj();

	transferQueue.SubmitCommandBuffer(vkTransferCmdBuffer, transferSync.GetFrontFence());
	transferSync.WaitForFrontFence();
	transferSync.ResetFrontFence();

	// Transition Images to Fragment Optimal
	VKCommandBuffer& graphicsCmdBuffer = terra.Graphics().CmdBuffer();
	graphicsCmdBuffer.ResetFirstBuffer();

	const VkCommandBuffer vkGraphicsCmdBuffer = graphicsCmdBuffer.GetFirstCommandBuffer();

	textures.AcquireOwnerShips(vkGraphicsCmdBuffer);
	engine.AcquireOwnerShipGraphics(vkGraphicsCmdBuffer);

	textures.TransitionImages(vkGraphicsCmdBuffer);

	graphicsCmdBuffer.CloseFirstBuffer();

	VkSyncObjects& graphicsSync = terra.Graphics().SyncObj();

	terra.Graphics().Que().SubmitCommandBuffer(vkGraphicsCmdBuffer, graphicsSync.GetFrontFence());
	graphicsSync.WaitForFrontFence();
	graphicsSync.ResetFrontFence();

	// Compute
	VKCommandBuffer& computeCmdBuffer = terra.Compute().CmdBuffer();
	computeCmdBuffer.ResetFirstBuffer();

	const VkCommandBuffer vkComputeCmdBuffer = computeCmdBuffer.GetFirstCommandBuffer();

	engine.AcquireOwnerShipCompute(vkComputeCmdBuffer);

	computeCmdBuffer.CloseFirstBuffer();

	VkSyncObjects& computeSync = terra.Compute().SyncObj();

	terra.Compute().Que().SubmitCommandBuffer(vkComputeCmdBuffer, computeSync.GetFrontFence());
	computeSync.WaitForFrontFence();
	computeSync.ResetFrontFence();

	textures.SetDescriptorLayouts();

	terra.GraphicsDesc().CreateDescriptorSets(logicalDevice);
	terra.ComputeDesc().CreateDescriptorSets(logicalDevice);

	engine.ConstructPipelines();

	// Cleanup Upload Buffers
	engine.ReleaseUploadResources();
	textures.ReleaseUploadBuffers();
	terra.Res().ResetUpload();
}

size_t RendererVK::AddTexture(
	std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
) {
	Terra& terra = Terra::Get();

	return terra.Texture().AddTexture(
		terra.Device().GetLogicalDevice(), std::move(textureData), width, height
	);
}

void RendererVK::WaitForAsyncTasks() {
	vkDeviceWaitIdle(Terra::Get().Device().GetLogicalDevice());
}
