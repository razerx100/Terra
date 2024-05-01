#ifndef RENDERER_VK_HPP_
#define RENDERER_VK_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <ThreadPool.hpp>

#include <Renderer.hpp>
#include <ISharedDataContainer.hpp>

class RendererVK final : public Renderer
{
public:
	RendererVK(
		const char* appName,
		void* windowHandle, void* moduleHandle,
		std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		ThreadPool& threadPool, ISharedDataContainer& sharedContainer,
		RenderEngineType engineType
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

	void AddModelSet(
		std::vector<std::shared_ptr<IModel>>&& models, const std::wstring& fragmentShader
	) override;
	void AddMeshletModelSet(
		std::vector<MeshletModel>&& meshletModels, const std::wstring& fragmentShader
	) override;
	void AddModelInputs(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) override;
	void AddModelInputs(
		std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gVerticesIndices,
		std::vector<std::uint32_t>&& gPrimIndices
	) override;

	void Update() override;
	void Render() override;
	void WaitForAsyncTasks() override;
	void ProcessData() override;

private:
	std::uint32_t m_width;
	std::uint32_t m_height;
};
#endif
