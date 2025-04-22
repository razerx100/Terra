#ifndef VK_RENDERER_VK_HPP_
#define VK_RENDERER_VK_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <array>
#include <ThreadPool.hpp>

#include <Renderer.hpp>
#include <Terra.hpp>

namespace Terra
{
template<class SurfaceManager_t, class DisplayManager_t, class RenderEngine_t>
class RendererVK final : public Renderer
{
public:
	RendererVK(
		const char* appName, void* windowHandle, void* moduleHandle,
		std::uint32_t width, std::uint32_t height, std::uint32_t bufferCount,
		std::shared_ptr<ThreadPool>&& threadPool
	) : m_terra{
			appName, windowHandle, moduleHandle, width, height, bufferCount,
			std::move(threadPool)
		}
	{}

	void FinaliseInitialisation() override
	{
		m_terra.FinaliseInitialisation();
	}

	void Resize(std::uint32_t width, std::uint32_t height) override
	{
		m_terra.Resize(width, height);
	}

	[[nodiscard]]
	Extent GetCurrentRenderingExtent() const noexcept override
	{
		const VkExtent2D currentRenderArea = m_terra.GetCurrentRenderArea();

		return Renderer::Extent
		{
			.width  = currentRenderArea.width,
			.height = currentRenderArea.height
		};
	}

	[[nodiscard]]
	Extent GetFirstDisplayCoordinates() const override
	{
		const VkExtent2D resolution = m_terra.GetFirstDisplayCoordinates();

		return Renderer::Extent{ .width = resolution.width, .height = resolution.height };
	}

	void SetShaderPath(const wchar_t* path) override
	{
		m_terra.GetRenderEngine().SetShaderPath(path);
	}

	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline) override
	{
		return m_terra.GetRenderEngine().AddGraphicsPipeline(gfxPipeline);
	}

	void ReconfigureModelPipelinesInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
		std::uint32_t increasedModelsPipelineIndex
	) override {
		m_terra.GetRenderEngine().ReconfigureModelPipelinesInBundle(
			modelBundleIndex, decreasedModelsPipelineIndex, increasedModelsPipelineIndex
		);
	}

	void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept override
	{
		m_terra.GetRenderEngine().RemoveGraphicsPipeline(pipelineIndex);
	}

	[[nodiscard]]
	size_t AddTexture(STexture&& texture) override
	{
		return m_terra.AddTextureAsCombined(std::move(texture));
	}

	void UnbindTexture(size_t textureIndex, std::uint32_t bindingIndex) override
	{
		m_terra.GetRenderEngine().UnbindCombinedTexture(textureIndex, bindingIndex);
	}

	[[nodiscard]]
	std::uint32_t BindTexture(size_t textureIndex) override
	{
		return m_terra.BindCombinedTexture(textureIndex);
	}

	void UnbindExternalTexture(std::uint32_t bindingIndex) override
	{
		m_terra.GetRenderEngine().UnbindExternalTexture(bindingIndex);
	}

	void RebindExternalTexture(size_t textureIndex, std::uint32_t bindingIndex) override
	{
		m_terra.GetRenderEngine().RebindExternalTexture(textureIndex, bindingIndex);
	}

	[[nodiscard]]
	std::uint32_t BindExternalTexture(size_t textureIndex) override
	{
		return m_terra.GetRenderEngine().BindExternalTexture(textureIndex);
	}

	void RemoveTexture(size_t textureIndex) override
	{
		m_terra.RemoveTexture(textureIndex);
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) override
	{
		return m_terra.AddModelBundle(std::move(modelBundle));
	}

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_terra.GetRenderEngine().RemoveModelBundle(bundleIndex);
	}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override
	{
		return m_terra.AddMeshBundle(std::move(meshBundle));
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_terra.GetRenderEngine().RemoveMeshBundle(bundleIndex);
	}

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera>&& camera) noexcept override
	{
		return m_terra.GetRenderEngine().AddCamera(std::move(camera));
	}

	void SetCamera(std::uint32_t index) noexcept override
	{
		m_terra.GetRenderEngine().SetCamera(index);
	}

	void RemoveCamera(std::uint32_t index) noexcept override
	{
		m_terra.GetRenderEngine().RemoveCamera(index);
	}

	void Render() override { m_terra.Render(); }

	void WaitForGPUToFinish() override { m_terra.WaitForGPUToFinish(); }

public:
	// External stuff
	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept override
	{
		return m_terra.GetRenderEngine().GetExternalResourceManager();
	}

	void UpdateExternalBufferDescriptor(
		const ExternalBufferBindingDetails& bindingDetails
	) override {
		m_terra.GetRenderEngine().UpdateExternalBufferDescriptor(bindingDetails);
	}

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData,
		size_t srcDataSizeInBytes, size_t dstBufferOffset
	) override {
		m_terra.GetRenderEngine().UploadExternalBufferGPUOnlyData(
			externalBufferIndex, std::move(cpuData), srcDataSizeInBytes, dstBufferOffset
		);
	}

	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset = 0, size_t srcDataSizeInBytes = 0
	) override {
		m_terra.GetRenderEngine().QueueExternalBufferGPUCopy(
			externalBufferSrcIndex, externalBufferDstIndex, dstBufferOffset, srcBufferOffset,
			srcDataSizeInBytes
		);
	}

	[[nodiscard]]
	std::uint32_t AddExternalRenderPass() override
	{
		return m_terra.GetRenderEngine().AddExternalRenderPass();
	}

	[[nodiscard]]
	ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept override
	{
		return m_terra.GetRenderEngine().GetExternalRenderPassRP(index);
	}

	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(
		size_t index
	) const noexcept override {
		return m_terra.GetRenderEngine().GetExternalRenderPassSP(index);
	}

	void SetSwapchainExternalRenderPass() override
	{
		m_terra.GetRenderEngine().SetSwapchainExternalRenderPass();
	}

	[[nodiscard]]
	ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept override
	{
		return m_terra.GetRenderEngine().GetSwapchainExternalRenderPassRP();
	}

	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept override
	{
		return m_terra.GetRenderEngine().GetSwapchainExternalRenderPassSP();
	}

	void RemoveExternalRenderPass(size_t index) noexcept override
	{
		m_terra.GetRenderEngine().RemoveExternalRenderPass(index);
	}

	void RemoveSwapchainExternalRenderPass() noexcept override
	{
		m_terra.GetRenderEngine().RemoveSwapchainExternalRenderPass();
	}

	[[nodiscard]]
	size_t GetActiveRenderPassCount() const noexcept override
	{
		return m_terra.GetRenderEngine().GetActiveRenderPassCount();
	}

	[[nodiscard]]
	ExternalFormat GetSwapchainFormat() const noexcept override
	{
		return m_terra.GetSwapchainFormat();
	}

private:
	Terra<SurfaceManager_t, DisplayManager_t, RenderEngine_t> m_terra;

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
}
#endif
