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

	void Resize(std::uint32_t width, std::uint32_t height) override;

	[[nodiscard]]
	Resolution GetFirstDisplayCoordinates() const override;

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept override;
	void SetShaderPath(const wchar_t* path) noexcept override;
	void AddPixelShader(const std::wstring& fragmentShader) override;
	void ChangePixelShader(std::uint32_t modelBundleID, const std::wstring& fragmentShader) override;

	[[nodiscard]]
	size_t AddTexture(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
	) override;
	void UnbindTexture(size_t index) override;
	void BindTexture(size_t index) override;
	void RemoveTexture(size_t index) override;

	[[nodiscard]]
	std::uint32_t AddModel(
		std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader
	) override;
	[[nodiscard]]
	std::uint32_t AddModel(
		std::shared_ptr<ModelMS>&& model, const std::wstring& fragmentShader
	) override;
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
	) override;
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const std::wstring& fragmentShader
	) override;
	void RemoveModelBundle(std::uint32_t bundleID) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;
	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle) override;
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

	void Render() override;

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
