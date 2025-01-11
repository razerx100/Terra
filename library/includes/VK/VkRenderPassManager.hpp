#ifndef VK_RENDER_PASS_MANAGER_HPP_
#define VK_RENDER_PASS_MANAGER_HPP_
#include <memory>
#include <PipelineManager.hpp>
#include <DepthBuffer.hpp>

// Render Pass Manager but without any actual render passes.
template<typename Pipeline_t>
class VkRenderPassManager
{
public:
	VkRenderPassManager(VkDevice device)
		: m_graphicsPipelineManager{ device }, m_depthBuffer{},
		m_colourAttachmentFormat{ VK_FORMAT_UNDEFINED }
	{}

	void SetShaderPath(const std::wstring& shaderPath) noexcept
	{
		m_graphicsPipelineManager.SetShaderPath(shaderPath);
	}

	void SetPipelineLayout(VkPipelineLayout graphicsPipelineLayout) noexcept
	{
		m_graphicsPipelineManager.SetPipelineLayout(graphicsPipelineLayout);
	}

	void SetColourAttachmentFormat(VkFormat colourAttachmentFormat) noexcept
	{
		m_colourAttachmentFormat = colourAttachmentFormat;
	}

	void SetDepthTesting(VkDevice device, MemoryManager* memoryManager)
	{
		m_depthBuffer = std::make_unique<DepthBuffer>(device, memoryManager);
	}

	void ResizeDepthBuffer(std::uint32_t width, std::uint32_t height)
	{
		if (m_depthBuffer)
			m_depthBuffer->Create(width, height);
	}

	void BeginRenderingWithDepth(
		const VKCommandBuffer& graphicsCmdBuffer, VkExtent2D renderArea,
		const VKImageView& colourAttachment, const VkClearColorValue& colourClearValue
	) const noexcept {
		VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

		// Depth
		DepthBuffer& depthBuffer = *m_depthBuffer;

		VkRenderingAttachmentInfo depthAttachmentInfo
		{
			.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView   = depthBuffer.GetVkView(),
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.clearValue  = VkClearValue{ .depthStencil = depthBuffer.GetClearValues() }
		};

		// Colour
		VkRenderingAttachmentInfo colourAttachmentInfo
		{
			.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView   = colourAttachment.GetView(),
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue  = VkClearValue{ .color = colourClearValue }
		};

		VkRenderingInfo renderingInfo
		{
			.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea           = VkRect2D{ .offset = VkOffset2D{ 0u, 0u }, .extent = renderArea },
			.layerCount           = 1u,
			.viewMask             = 0u,
			.colorAttachmentCount = 1u,
			.pColorAttachments    = &colourAttachmentInfo,
			.pDepthAttachment     = &depthAttachmentInfo
		};

		// Barrier and API call.
		VkImageBarrier2<2>{}
		.AddMemoryBarrier(
			ImageBarrierBuilder{}
			.Image(depthBuffer.GetView())
			// The scope would be from one Early Fragment Tests to the next one.
			.StageMasks(
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
			).AccessMasks(0u, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
			.Layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
		).AddMemoryBarrier(
			ImageBarrierBuilder{}
			.Image(colourAttachment)
			.StageMasks(
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
			).AccessMasks(0u, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
			.Layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		).RecordBarriers(cmdBuffer);

		vkCmdBeginRendering(cmdBuffer, &renderingInfo);
	}

	void BeginRendering(
		const VKCommandBuffer& graphicsCmdBuffer, VkExtent2D renderArea,
		const VKImageView& colourAttachment, const VkClearColorValue& colourClearValue
	) const noexcept {
		VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

		VkRenderingAttachmentInfo colourAttachmentInfo
		{
			.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView   = colourAttachment.GetView(),
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue  = VkClearValue{ .color = colourClearValue }
		};

		VkRenderingInfo renderingInfo
		{
			.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea           = VkRect2D{ .offset = VkOffset2D{ 0u, 0u }, .extent = renderArea },
			.layerCount           = 1u,
			.viewMask             = 0u,
			.colorAttachmentCount = 1u,
			.pColorAttachments    = &colourAttachmentInfo
		};

		VkImageBarrier2<1>{}.AddMemoryBarrier(
			ImageBarrierBuilder{}
			.Image(colourAttachment)
			.StageMasks(
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
			).AccessMasks(0u, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
			.Layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		).RecordBarriers(cmdBuffer);

		vkCmdBeginRendering(cmdBuffer, &renderingInfo);
	}

	void EndRendering(
		const VKCommandBuffer& graphicsCmdBuffer, const VKImageView& colourAttachment
	) const noexcept {
		VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

		vkCmdEndRendering(cmdBuffer);

		VkImageBarrier2<1>{}.AddMemoryBarrier(
			ImageBarrierBuilder{}
			.Image(colourAttachment)
			.StageMasks(
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
			).AccessMasks(VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0u)
			.Layouts(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		).RecordBarriers(cmdBuffer);
	}

	std::uint32_t AddOrGetGraphicsPipeline(const ShaderName& fragmentShader)
	{
		return GetGraphicsPSOIndex(fragmentShader);
	}

	void RecreatePipelines()
	{
		m_graphicsPipelineManager.RecreateAllGraphicsPipelines(
			m_colourAttachmentFormat, GetDepthStencilFormat()
		);
	}

	[[nodiscard]]
	const PipelineManager<Pipeline_t>& GetGraphicsPipelineManager() const noexcept
	{
		return m_graphicsPipelineManager;
	}

private:
	[[nodiscard]]
	DepthStencilFormat GetDepthStencilFormat() const noexcept
	{
		DepthStencilFormat depthStencilFormat
		{
			.depthFormat   = VK_FORMAT_UNDEFINED,
			.stencilFormat = VK_FORMAT_UNDEFINED
		};

		if (m_depthBuffer)
			depthStencilFormat.depthFormat = m_depthBuffer->GetFormat();

		return depthStencilFormat;
	}

	[[nodiscard]]
	std::uint32_t GetGraphicsPSOIndex(const ShaderName& fragmentShader)
	{
		std::optional<std::uint32_t> oPSOIndex = m_graphicsPipelineManager.TryToGetPSOIndex(
			fragmentShader
		);

		auto psoIndex = std::numeric_limits<std::uint32_t>::max();

		if (!oPSOIndex)
		{
			const DepthStencilFormat depthStencilFormat = GetDepthStencilFormat();

			psoIndex = m_graphicsPipelineManager.AddGraphicsPipeline(
				fragmentShader, m_colourAttachmentFormat, depthStencilFormat
			);
		}
		else
			psoIndex = oPSOIndex.value();

		return psoIndex;
	}

private:
	PipelineManager<Pipeline_t>  m_graphicsPipelineManager;
	std::unique_ptr<DepthBuffer> m_depthBuffer;
	VkFormat                     m_colourAttachmentFormat;

public:
	VkRenderPassManager(const VkRenderPassManager&) = delete;
	VkRenderPassManager& operator=(const VkRenderPassManager&) = delete;

	VkRenderPassManager(VkRenderPassManager&& other) noexcept
		: m_graphicsPipelineManager{ std::move(other.m_graphicsPipelineManager) },
		m_depthBuffer{ std::move(other.m_depthBuffer) },
		m_colourAttachmentFormat{ other.m_colourAttachmentFormat }
	{}
	VkRenderPassManager& operator=(VkRenderPassManager&& other) noexcept
	{
		m_graphicsPipelineManager = std::move(other.m_graphicsPipelineManager);
		m_depthBuffer             = std::move(other.m_depthBuffer);
		m_colourAttachmentFormat  = other.m_colourAttachmentFormat;

		return *this;
	}
};
#endif
