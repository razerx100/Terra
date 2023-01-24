#ifndef RENDER_ENGINE_BASE_HPP_
#define RENDER_ENGINE_BASE_HPP_
#include <memory>
#include <concepts>
#include <span>
#include <PipelineLayout.hpp>
#include <GraphicsPipelineBase.hpp>
#include <RenderEngine.hpp>
#include <VKRenderPass.hpp>
#include <ViewportAndScissorManager.hpp>
#include <DepthBuffer.hpp>

class RenderEngineBase : public RenderEngine {
public:
	RenderEngineBase(VkDevice device) noexcept;

	void Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) final;
	void ExecutePostRenderStage() final;
	void CreateRenderPass(VkDevice device, VkFormat swapchainFormat) final;
	void CreateDepthBuffer(VkDevice device, std::uint32_t width, std::uint32_t height) final;
	void ResizeViewportAndScissor(std::uint32_t width, std::uint32_t height) noexcept final;
	void CleanUpDepthBuffer() noexcept final;

	[[nodiscard]]
	VkImageView GetDepthImageView() const noexcept final;
	[[nodiscard]]
	VkRenderPass GetRenderPass() const noexcept final;

protected:
	void ExecutePreGraphicsStage(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex);
	void BindGraphicsDescriptorSets(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex);

	void ConstructGraphicsPipelineLayout(VkDevice device, std::uint32_t frameCount);

	[[nodiscard]]
	std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept;

	using WaitSemaphoreData = std::pair<std::span<VkSemaphore>, std::span<VkPipelineStageFlags>>;

	virtual WaitSemaphoreData GetWaitSemaphores() const noexcept = 0;

	template<std::derived_from<GraphicsPipelineBase> Pipeline>
	void CreateGraphicsPipelines(
		VkDevice device, std::unique_ptr<Pipeline>& graphicsPipeline0,
		std::vector<std::unique_ptr<Pipeline>>& graphicsPipelines
	) const noexcept {
		VkPipelineLayout graphicsLayout = m_graphicsPipelineLayout->GetLayout();

		graphicsPipeline0->CreateGraphicsPipeline(
			device, graphicsLayout, GetRenderPass(), m_shaderPath
		);
		for (auto& graphicsPipeline : graphicsPipelines)
			graphicsPipeline->CreateGraphicsPipeline(
				device, graphicsLayout, GetRenderPass(), m_shaderPath
			);
	}

private:
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	VKRenderPass m_renderPass;
	ViewportAndScissorManager m_viewportAndScissor;
	DepthBuffer m_depthBuffer;
};
#endif
