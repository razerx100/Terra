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
	Extent GetCurrentRenderingExtent() const noexcept override;

	[[nodiscard]]
	Extent GetFirstDisplayCoordinates() const override;

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept override;
	void SetShaderPath(const wchar_t* path) override;

	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline) override;

	void ChangeModelPipelineInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) override;
	void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept override;

	[[nodiscard]]
	size_t AddTexture(STexture&& texture) override;

	void UnbindTexture(size_t index) override;

	[[nodiscard]]
	std::uint32_t BindTexture(size_t index) override;

	void RemoveTexture(size_t index) override;

	[[nodiscard]]
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override;

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept override;

	void SetCamera(std::uint32_t index) noexcept override;
	void RemoveCamera(std::uint32_t index) noexcept override;

	void Render() override;
	void WaitForGPUToFinish() override;

public:
	// External stuff
	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept override;

	void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails) override;

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	) override;
	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset = 0, size_t srcDataSizeInBytes = 0
	) override;

	[[nodiscard]]
	std::uint32_t AddExternalRenderPass() override;
	[[nodiscard]]
	ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept override;
	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(
		size_t index
	) const noexcept override;

	void SetSwapchainExternalRenderPass() override;

	[[nodiscard]]
	ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept override;
	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept override;

	void RemoveExternalRenderPass(size_t index) noexcept override;
	void RemoveSwapchainExternalRenderPass() noexcept override;

	[[nodiscard]]
	size_t GetActiveRenderPassCount() const noexcept override;

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
