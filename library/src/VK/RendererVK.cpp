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

void RendererVK::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept
{
	m_terra.GetRenderEngine().SetBackgroundColour(colourVector);
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

Renderer::Resolution RendererVK::GetFirstDisplayCoordinates() const
{
	DisplayManager::Resolution resolution = m_terra.GetFirstDisplayCoordinates();

	return Renderer::Resolution{ .width = resolution.width, .height = resolution.height };
}

void RendererVK::SetShaderPath(const wchar_t* path)
{
	m_terra.GetRenderEngine().SetShaderPath(path);
}

void RendererVK::AddPixelShader(const ShaderName& fragmentShader)
{
	m_terra.GetRenderEngine().AddFragmentShader(fragmentShader);
}

void RendererVK::ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& fragmentShader)
{
	m_terra.GetRenderEngine().ChangeFragmentShader(modelBundleID, fragmentShader);
}

void RendererVK::MakePixelShaderRemovable(const ShaderName& fragmentShader) noexcept
{
	m_terra.GetRenderEngine().MakeFragmentShaderRemovable(fragmentShader);
}

size_t RendererVK::AddTexture(STexture&& texture)
{
	return m_terra.AddTextureAsCombined(std::move(texture));
}

void RendererVK::UnbindTexture(size_t index)
{
	m_terra.GetRenderEngine().UnbindCombinedTexture(index);
}

std::uint32_t RendererVK::BindTexture(size_t index)
{
	return m_terra.BindCombinedTexture(index);
}

void RendererVK::RemoveTexture(size_t index)
{
	m_terra.RemoveTexture(index);
}

std::uint32_t RendererVK::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader
) {
	return m_terra.AddModelBundle(std::move(modelBundle), fragmentShader);
}

void RendererVK::RemoveModelBundle(std::uint32_t bundleID) noexcept
{
	m_terra.GetRenderEngine().RemoveModelBundle(bundleID);
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
