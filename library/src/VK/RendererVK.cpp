#include <array>

#include <RendererVK.hpp>

RendererVK::RendererVK(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint32_t bufferCount,
	std::shared_ptr<ThreadPool>&& threadPool,
	RenderEngineType engineType
// The terra object is quite big but a renderer object would always be a heap allocated
// object, so it should be fine.
) : m_terra{
		appName, windowHandle, moduleHandle, width, height, bufferCount, std::move(threadPool),
		engineType
	}
{}

void RendererVK::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept
{
	m_terra.GetRenderEngine().SetBackgroundColour(colourVector);
}

/*void RendererVK::AddModelSet(
	std::vector<std::shared_ptr<Model>>&& models, const std::wstring& fragmentShader
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
}*/

void RendererVK::Update()
{
	//Terra& terra = Terra::Get();

	/*terra.Swapchain().QueryNextImageIndex(
		terra.Device().GetLogicalDevice(),
		terra.Graphics().SyncObj().GetFrontSemaphore()
	);*/
	//const size_t imageIndex = terra.Swapchain().GetNextImageIndex();

	//terra.Engine().UpdateModelBuffers(static_cast<VkDeviceSize>(imageIndex));
}

void RendererVK::Render()
{
	m_terra.Render();
}

void RendererVK::Resize(std::uint32_t width, std::uint32_t height)
{
	m_terra.Resize(width, height);
}

Renderer::Resolution RendererVK::GetFirstDisplayCoordinates() const
{
	DisplayManager::Resolution resolution = m_terra.GetFirstDisplayCoordinates();

	return Renderer::Resolution{ .width = resolution.width, .height = resolution.height };
}

void RendererVK::SetShaderPath(const wchar_t* path) noexcept
{
	m_terra.GetRenderEngine().SetShaderPath(path);
}

void RendererVK::ProcessData()
{
	//Terra& terra = Terra::Get();

	//const VkDevice logicalDevice = terra.Device().GetLogicalDevice();

	//BufferManager& buffers = terra.Buffers();
	//RenderEngine& engine = terra.Engine();

	// Create Buffers
	//buffers.CreateBuffers(logicalDevice);
	//engine.CreateBuffers(logicalDevice);

	/*
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
	*/

	/*
	// Set Upload Memory Start
	UploadContainer& uploadContainer = terra.Res().UploadCont();
	uploadContainer.SetMemoryStart(uploadMem.GetMappedCPUPtr());
	*/

	// Bind Buffers to memory
	//buffers.BindResourceToMemory(logicalDevice);
	//engine.BindResourcesToMemory(logicalDevice);
	//terra.Texture().BindMemories(logicalDevice);

	// Async Copy
	std::atomic_size_t works = 0u;

	//uploadContainer.CopyData(works);
	//engine.CopyData();

	while (works != 0u);

	/*
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

	textures.SetDescriptorLayouts();*/

	//terra.GraphicsDesc().CreateDescriptorSets(logicalDevice);
	//terra.ComputeDesc().CreateDescriptorSets(logicalDevice);

	//engine.ConstructPipelines();

	// Cleanup Upload Buffers
	//engine.ReleaseUploadResources();
	//textures.ReleaseUploadBuffers();
	//terra.Res().ResetUpload();
}

size_t RendererVK::AddTexture(
	std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
) {
	return m_terra.GetRenderEngine().AddTextureAsCombined(std::move(textureData), width, height);
}

void RendererVK::WaitForAsyncTasks() {
	//vkDeviceWaitIdle(Terra::Get().Device().GetLogicalDevice());
}

void RendererVK::AddModel(std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader)
{
	m_terra.GetRenderEngine().AddModel(std::move(model), fragmentShader);
}

void RendererVK::AddModel(std::shared_ptr<ModelMS>&& model, const std::wstring& fragmentShader)
{
	m_terra.GetRenderEngine().AddModel(std::move(model), fragmentShader);
}

void RendererVK::AddModelBundle(
	std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
) {
	m_terra.GetRenderEngine().AddModelBundle(std::move(modelBundle), fragmentShader);
}

void RendererVK::AddModelBundle(
	std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const std::wstring& fragmentShader
) {
	m_terra.GetRenderEngine().AddModelBundle(std::move(modelBundle), fragmentShader);
}

void RendererVK::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	m_terra.GetRenderEngine().AddMeshBundle(std::move(meshBundle));
}

void RendererVK::AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle)
{
	m_terra.GetRenderEngine().AddMeshBundle(std::move(meshBundle));
}

void RendererVK::AddMaterial(std::shared_ptr<Material> material)
{
	m_terra.GetRenderEngine().AddMaterial(std::move(material));
}

void RendererVK::AddMaterials(std::vector<std::shared_ptr<Material>>&& materials)
{
	m_terra.GetRenderEngine().AddMaterials(std::move(materials));
}

void RendererVK::SetCamera(std::shared_ptr<Camera>&& camera)
{
	RenderEngine& renderEngine = m_terra.GetRenderEngine();

	const std::uint32_t cameraIndex = renderEngine.AddCamera(std::move(camera));
	renderEngine.SetCamera(cameraIndex);
}
