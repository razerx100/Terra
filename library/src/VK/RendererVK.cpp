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

void RendererVK::AddPixelShader(const std::wstring& fragmentShader)
{
	m_terra.GetRenderEngine().AddFragmentShader(fragmentShader);
}

void RendererVK::ChangePixelShader(std::uint32_t modelBundleID, const std::wstring& fragmentShader)
{
	m_terra.GetRenderEngine().ChangeFragmentShader(modelBundleID, fragmentShader);
}

size_t RendererVK::AddTexture(
	std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
) {
	return m_terra.GetRenderEngine().AddTextureAsCombined(std::move(textureData), width, height);
}

void RendererVK::UnbindTexture(size_t index)
{
	m_terra.GetRenderEngine().UnbindCombinedTexture(index);
}

void RendererVK::BindTexture(size_t index)
{
	m_terra.GetRenderEngine().BindCombinedTexture(index);
}

void RendererVK::RemoveTexture(size_t index)
{
	m_terra.GetRenderEngine().RemoveTexture(index);
}

std::uint32_t RendererVK::AddModel(std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader)
{
	return m_terra.GetRenderEngine().AddModel(std::move(model), fragmentShader);
}

std::uint32_t RendererVK::AddModel(std::shared_ptr<ModelMS>&& model, const std::wstring& fragmentShader)
{
	return m_terra.GetRenderEngine().AddModel(std::move(model), fragmentShader);
}

std::uint32_t RendererVK::AddModelBundle(
	std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
) {
	return m_terra.GetRenderEngine().AddModelBundle(std::move(modelBundle), fragmentShader);
}

std::uint32_t RendererVK::AddModelBundle(
	std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const std::wstring& fragmentShader
) {
	return m_terra.GetRenderEngine().AddModelBundle(std::move(modelBundle), fragmentShader);
}

void RendererVK::RemoveModelBundle(std::uint32_t bundleID) noexcept
{
	m_terra.GetRenderEngine().RemoveModelBundle(bundleID);
}

std::uint32_t RendererVK::AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle)
{
	return m_terra.GetRenderEngine().AddMeshBundle(std::move(meshBundle));
}

std::uint32_t RendererVK::AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle)
{
	return m_terra.GetRenderEngine().AddMeshBundle(std::move(meshBundle));
}

void RendererVK::RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
{
	m_terra.GetRenderEngine().RemoveMeshBundle(bundleIndex);
}

size_t RendererVK::AddMaterial(std::shared_ptr<Material> material)
{
	return m_terra.GetRenderEngine().AddMaterial(std::move(material));
}

std::vector<size_t> RendererVK::AddMaterials(std::vector<std::shared_ptr<Material>>&& materials)
{
	return m_terra.GetRenderEngine().AddMaterials(std::move(materials));
}

void RendererVK::UpdateMaterial(size_t index) const noexcept
{
	m_terra.GetRenderEngine().UpdateMaterial(index);
}

void RendererVK::RemoveMaterial(size_t index) noexcept
{
	m_terra.GetRenderEngine().RemoveMaterial(index);
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
