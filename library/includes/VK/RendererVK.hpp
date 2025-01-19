#ifndef RENDERER_VK_HPP_
#define RENDERER_VK_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <ThreadPool.hpp>

#include <Renderer.hpp>
#include <Terra.hpp>

class RendererVK final : public Renderer
{
public:
	RendererVK(
		const char* appName,
		void* windowHandle, void* moduleHandle,
		std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		std::shared_ptr<ThreadPool>&& threadPool, RenderEngineType engineType
	);

	void FinaliseInitialisation() override;

	void Resize(std::uint32_t width, std::uint32_t height) override;

	[[nodiscard]]
	Resolution GetFirstDisplayCoordinates() const override;

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept override;
	void SetShaderPath(const wchar_t* path) override;
	void AddPixelShader(const ShaderName& fragmentShader) override;
	void ChangePixelShader(std::uint32_t modelBundleID, const ShaderName& fragmentShader) override;

	[[nodiscard]]
	// The returned Index is the texture's ID. Not its index in the shader. It should be
	// used to remove or bind the texture.
	size_t AddTexture(STexture&& texture) override;
	void UnbindTexture(size_t index) override;
	[[nodiscard]]
	// The returned index is the index of the texture in the shader.
	std::uint32_t BindTexture(size_t index) override;
	void RemoveTexture(size_t index) override;

	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader
	) override;
	void RemoveModelBundle(std::uint32_t bundleID) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;
	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	size_t AddMaterial(std::shared_ptr<Material> material) override;
	[[nodiscard]]
	std::vector<size_t> AddMaterials(std::vector<std::shared_ptr<Material>>&& materials) override;
	void UpdateMaterial(size_t index) const noexcept override;
	void RemoveMaterial(size_t index) noexcept override;

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept override;
	void SetCamera(std::uint32_t index) noexcept override;
	void RemoveCamera(std::uint32_t index) noexcept override;

	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept override;

	void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails) override;

	void Render() override;
	void WaitForGPUToFinish() override;

private:
	Terra m_terra;

public:
	RendererVK(const RendererVK&) = delete;
	RendererVK& operator=(const RendererVK&) = delete;

	RendererVK(RendererVK&& other) noexcept
		: m_terra{ std::move(other.m_terra) }
	{}
	RendererVK& operator=(RendererVK&& other) noexcept
	{
		m_terra = std::move(other.m_terra);

		return *this;
	}
};
#endif
