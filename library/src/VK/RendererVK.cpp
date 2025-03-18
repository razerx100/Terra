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

void RendererVK::FinaliseInitialisation()
{
	m_terra.FinaliseInitialisation();
}

void RendererVK::Render()
{
	m_terra.Render();
}

void RendererVK::WaitForGPUToFinish()
{
	m_terra.WaitForGPUToFinish();
}

void RendererVK::Resize(std::uint32_t width, std::uint32_t height)
{
	m_terra.Resize(width, height);
}

Renderer::Extent RendererVK::GetCurrentRenderingExtent() const noexcept
{
	VkExtent2D currentRenderArea = m_terra.GetCurrentRenderArea();

	return Renderer::Extent{ .width = currentRenderArea.width, .height = currentRenderArea.height };
}

Renderer::Extent RendererVK::GetFirstDisplayCoordinates() const
{
	DisplayManager::Resolution resolution = m_terra.GetFirstDisplayCoordinates();

	return Renderer::Extent{ .width = resolution.width, .height = resolution.height };
}

void RendererVK::SetShaderPath(const wchar_t* path)
{
	m_terra.GetRenderEngine().SetShaderPath(path);
}

std::uint32_t RendererVK::AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline)
{
	return m_terra.GetRenderEngine().AddGraphicsPipeline(gfxPipeline);
}

void RendererVK::ChangeModelPipelineInBundle(
	std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
	std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
) {
	m_terra.GetRenderEngine().ChangeModelPipelineInBundle(
		modelBundleIndex, modelIndex, oldPipelineIndex, newPipelineIndex
	);
}

void RendererVK::RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept
{
	m_terra.GetRenderEngine().RemoveGraphicsPipeline(pipelineIndex);
}

size_t RendererVK::AddTexture(STexture&& texture)
{
	return m_terra.AddTextureAsCombined(std::move(texture));
}

void RendererVK::UnbindTexture(size_t textureIndex)
{
	m_terra.GetRenderEngine().UnbindCombinedTexture(textureIndex);
}

std::uint32_t RendererVK::BindTexture(size_t textureIndex)
{
	return m_terra.BindCombinedTexture(textureIndex);
}

void RendererVK::UnbindExternalTexture(size_t textureIndex)
{
	m_terra.GetRenderEngine().UnbindExternalTexture(textureIndex);
}

void RendererVK::RebindExternalTexture(size_t textureIndex, std::uint32_t bindingIndex)
{
	m_terra.GetRenderEngine().RebindExternalTexture(textureIndex, bindingIndex);
}

std::uint32_t RendererVK::BindExternalTexture(size_t textureIndex)
{
	return m_terra.GetRenderEngine().BindExternalTexture(textureIndex);
}

void RendererVK::RemoveTexture(size_t textureIndex)
{
	m_terra.RemoveTexture(textureIndex);
}

std::uint32_t RendererVK::AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
{
	return m_terra.AddModelBundle(std::move(modelBundle));
}

void RendererVK::RemoveModelBundle(std::uint32_t bundleIndex) noexcept
{
	m_terra.GetRenderEngine().RemoveModelBundle(bundleIndex);
}

std::uint32_t RendererVK::AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
{
	return m_terra.AddMeshBundle(std::move(meshBundle));
}

void RendererVK::RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
{
	m_terra.GetRenderEngine().RemoveMeshBundle(bundleIndex);
}

std::uint32_t RendererVK::AddCamera(std::shared_ptr<Camera>&& camera) noexcept
{
	return m_terra.GetRenderEngine().AddCamera(std::move(camera));
}

void RendererVK::SetCamera(std::uint32_t index) noexcept
{
	m_terra.GetRenderEngine().SetCamera(index);
}

void RendererVK::RemoveCamera(std::uint32_t index) noexcept
{
	m_terra.GetRenderEngine().RemoveCamera(index);
}

ExternalResourceManager* RendererVK::GetExternalResourceManager() noexcept
{
	return m_terra.GetRenderEngine().GetExternalResourceManager();
}

void RendererVK::UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails)
{
	m_terra.GetRenderEngine().UpdateExternalBufferDescriptor(bindingDetails);
}

void RendererVK::UploadExternalBufferGPUOnlyData(
	std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData,
	size_t srcDataSizeInBytes, size_t dstBufferOffset
) {
	m_terra.GetRenderEngine().UploadExternalBufferGPUOnlyData(
		externalBufferIndex, std::move(cpuData), srcDataSizeInBytes, dstBufferOffset
	);
}

void RendererVK::QueueExternalBufferGPUCopy(
	std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
	size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
) {
	m_terra.GetRenderEngine().QueueExternalBufferGPUCopy(
		externalBufferSrcIndex, externalBufferDstIndex, dstBufferOffset, srcBufferOffset,
		srcDataSizeInBytes
	);
}

std::uint32_t RendererVK::AddExternalRenderPass()
{
	return m_terra.GetRenderEngine().AddExternalRenderPass();
}

ExternalRenderPass* RendererVK::GetExternalRenderPassRP(size_t index) const noexcept
{
	return m_terra.GetRenderEngine().GetExternalRenderPassRP(index);
}

std::shared_ptr<ExternalRenderPass> RendererVK::GetExternalRenderPassSP(
	size_t index
) const noexcept {
	return m_terra.GetRenderEngine().GetExternalRenderPassSP(index);
}

void RendererVK::SetSwapchainExternalRenderPass()
{
	m_terra.GetRenderEngine().SetSwapchainExternalRenderPass();
}

ExternalRenderPass* RendererVK::GetSwapchainExternalRenderPassRP() const noexcept
{
	return m_terra.GetRenderEngine().GetSwapchainExternalRenderPassRP();
}

std::shared_ptr<ExternalRenderPass> RendererVK::GetSwapchainExternalRenderPassSP() const noexcept
{
	return m_terra.GetRenderEngine().GetSwapchainExternalRenderPassSP();
}

void RendererVK::RemoveExternalRenderPass(size_t index) noexcept
{
	m_terra.GetRenderEngine().RemoveExternalRenderPass(index);
}

void RendererVK::RemoveSwapchainExternalRenderPass() noexcept
{
	m_terra.GetRenderEngine().RemoveSwapchainExternalRenderPass();
}

size_t RendererVK::GetActiveRenderPassCount() const noexcept
{
	return m_terra.GetRenderEngine().GetActiveRenderPassCount();
}

ExternalFormat RendererVK::GetSwapchainFormat() const noexcept
{
	return m_terra.GetSwapchainFormat();
}
