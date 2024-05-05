#ifndef RENDERER_VK_HPP_
#define RENDERER_VK_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <ThreadPool.hpp>

#include <Renderer.hpp>

class RendererVK final : public Renderer
{
public:
	RendererVK(
		const char* appName,
		void* windowHandle, void* moduleHandle,
		std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		ThreadPool& threadPool, RenderEngineType engineType
	);

	void Resize(std::uint32_t width, std::uint32_t height) override;

	[[nodiscard]]
	Resolution GetFirstDisplayCoordinates() const override;

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept override;
	void SetShaderPath(const wchar_t* path) noexcept override;

	[[nodiscard]]
	size_t AddTexture(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height
	) override;

	void AddModel(std::shared_ptr<ModelVS>&& model, const std::wstring& pixelShader) override;
	void AddModel(std::shared_ptr<ModelMS>&& model, const std::wstring& pixelShader) override;
	void AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& pixelShader
	) override;
	void AddModelBundle(
		std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const std::wstring& pixelShader
	) override;

	void AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;
	void AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle) override;

	void AddMaterial(std::shared_ptr<Material> material) override;
	void AddMaterials(std::vector<std::shared_ptr<Material>>&& materials) override;

	void SetCamera(std::shared_ptr<Camera>&& camera) override;

	void Update() override;
	void Render() override;
	void WaitForAsyncTasks() override;
	void ProcessData() override;

private:
	std::uint32_t m_width;
	std::uint32_t m_height;
};
#endif
