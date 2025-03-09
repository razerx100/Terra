#ifndef VK_RENDER_PASS_MANAGER_HPP_
#define VK_RENDER_PASS_MANAGER_HPP_
#include <memory>
#include <array>
#include <vector>
#include <PipelineManager.hpp>

// This class shouldn't be created and destroyed on every frame.
class RenderingInfoBuilder
{
public:
	RenderingInfoBuilder()
		: m_colourAttachments{},
		m_depthAttachment{
			.sType     = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = VK_NULL_HANDLE
		},
		m_stencilAttachment{
			.sType     = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = VK_NULL_HANDLE
		}
	{}
	RenderingInfoBuilder(size_t colourAttachmentCount) : RenderingInfoBuilder{}
	{
		m_colourAttachments.reserve(colourAttachmentCount);
	}

	RenderingInfoBuilder& AddColourAttachment(
		const VKImageView& colourView, const VkClearColorValue& clearValue,
		VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOP
	) noexcept {
		m_colourAttachments.emplace_back(
			VkRenderingAttachmentInfo
			{
				.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView   = colourView.GetView(),
				.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.resolveMode = VK_RESOLVE_MODE_NONE,
				.loadOp      = loadOp,
				.storeOp     = storeOP,
				.clearValue  = VkClearValue{ .color = clearValue }
			}
		);

		return *this;
	}

	// These functions can be used every frame.
	RenderingInfoBuilder& SetDepthAttachment(
		const VKImageView& depthView, const VkClearDepthStencilValue& clearValue,
		VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOP
	) noexcept {
		m_depthAttachment.imageView   = depthView.GetView();
		m_depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		m_depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		m_depthAttachment.loadOp      = loadOp;
		m_depthAttachment.storeOp     = storeOP;
		m_depthAttachment.clearValue  = VkClearValue{ .depthStencil = clearValue };

		return *this;
	}
	RenderingInfoBuilder& SetDepthView(const VKImageView& depthView) noexcept
	{
		m_depthAttachment.imageView = depthView.GetView();

		return *this;
	}
	RenderingInfoBuilder& SetDepthClearColour(const VkClearDepthStencilValue& clearValue
	) noexcept {
		m_depthAttachment.clearValue = VkClearValue{ .depthStencil = clearValue };

		return *this;
	}

	RenderingInfoBuilder& SetStencilAttachment(
		const VKImageView& stencilView, const VkClearDepthStencilValue& clearValue,
		VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOP
	) noexcept {
		m_stencilAttachment.imageView   = stencilView.GetView();
		m_stencilAttachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
		m_stencilAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		m_stencilAttachment.loadOp      = loadOp;
		m_stencilAttachment.storeOp     = storeOP;
		m_stencilAttachment.clearValue  = VkClearValue{ .depthStencil = clearValue };

		return *this;
	}
	RenderingInfoBuilder& SetStencilView(const VKImageView& stencilView) noexcept
	{
		m_stencilAttachment.imageView = stencilView.GetView();

		return *this;
	}
	RenderingInfoBuilder& SetStencilClearColour(const VkClearDepthStencilValue& clearValue) noexcept
	{
		m_stencilAttachment.clearValue = VkClearValue{ .depthStencil = clearValue };

		return *this;
	}

	RenderingInfoBuilder& SetColourView(
		size_t colourAttachmentIndex, const VKImageView& colourView
	) noexcept {
		VkRenderingAttachmentInfo& colourAttachment = m_colourAttachments[colourAttachmentIndex];

		colourAttachment.imageView  = colourView.GetView();

		return *this;
	}
	RenderingInfoBuilder& SetColourClearValue(
		size_t colourAttachmentIndex, const VkClearColorValue& clearValue
	) noexcept {
		VkRenderingAttachmentInfo& colourAttachment = m_colourAttachments[colourAttachmentIndex];

		colourAttachment.clearValue = VkClearValue{.color = clearValue };

		return *this;
	}

	VkRenderingInfo BuildRenderingInfo(VkExtent2D renderArea) const noexcept
	{
		return VkRenderingInfo
		{
			.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea           = VkRect2D{ .offset = VkOffset2D{ 0u, 0u }, .extent = renderArea },
			.layerCount           = 1u,
			.viewMask             = 0u,
			.colorAttachmentCount = static_cast<std::uint32_t>(std::size(m_colourAttachments)),
			.pColorAttachments    = std::data(m_colourAttachments),
			// As long the depthImage and stencilImage handles are VK_NULL_HANDLE, including them
			// even in pipelines which don't use any depthStencil should be fine.
			.pDepthAttachment     = &m_depthAttachment,
			.pStencilAttachment   = &m_stencilAttachment
		};
	}

private:
	std::vector<VkRenderingAttachmentInfo> m_colourAttachments;
	VkRenderingAttachmentInfo              m_depthAttachment;
	VkRenderingAttachmentInfo              m_stencilAttachment;

public:
	RenderingInfoBuilder(const RenderingInfoBuilder& other) noexcept
		: m_colourAttachments{ other.m_colourAttachments },
		m_depthAttachment{ other.m_depthAttachment },
		m_stencilAttachment{ other.m_stencilAttachment }
	{}

	RenderingInfoBuilder& operator=(const RenderingInfoBuilder& other) noexcept
	{
		m_colourAttachments = other.m_colourAttachments;
		m_depthAttachment   = other.m_depthAttachment;
		m_stencilAttachment = other.m_stencilAttachment;

		return *this;
	}

	RenderingInfoBuilder(RenderingInfoBuilder&& other) noexcept
		: m_colourAttachments{ std::move(other.m_colourAttachments) },
		m_depthAttachment{ other.m_depthAttachment },
		m_stencilAttachment{ other.m_stencilAttachment }
	{}

	RenderingInfoBuilder& operator=(RenderingInfoBuilder&& other) noexcept
	{
		m_colourAttachments = std::move(other.m_colourAttachments);
		m_depthAttachment   = other.m_depthAttachment;
		m_stencilAttachment = other.m_stencilAttachment;

		return *this;
	}
};

// Render Pass Manager but without any actual render passe object. Encapsulates the concept of
// a Render pass only.
class VkRenderPassManager
{
public:
	VkRenderPassManager() : m_renderingInfoBuilder{}, m_startImageBarriers{} {}

	void AddColourAttachment(
		const VKImageView& colourView, const VkClearColorValue& clearValue,
		VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOP
	) noexcept {
		m_renderingInfoBuilder.AddColourAttachment(colourView, clearValue, loadOp, storeOP);
	}

	[[nodiscard]]
	// The Stage masks will be overwritten.
	std::uint32_t AddColourStartBarrier(ImageBarrierBuilder& barrierBuilder) noexcept
	{
		barrierBuilder.StageMasks(
			// The top of the pipeline bit doesn't queue a wait operation. So, have to wait for
			// the threads from the previous Colour attachment output.
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
		);

		return AddStartImageBarrier(barrierBuilder);
	}
	[[nodiscard]]
	// The Stage masks will be overwritten.
	std::uint32_t AddColourStartBarrier(ImageBarrierBuilder&& barrierBuilder) noexcept
	{
		return AddColourStartBarrier(barrierBuilder);
	}

	[[nodiscard]]
	// The Stage masks will be overwritten.
	std::uint32_t AddDepthOrStencilStartBarrier(ImageBarrierBuilder& barrierBuilder) noexcept
	{
		// Wait for all the threads from one Early Fragment Tests to the next one.
		barrierBuilder.StageMasks(
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
		);

		return AddStartImageBarrier(barrierBuilder);
	}
	[[nodiscard]]
	// The Stage masks will be overwritten.
	std::uint32_t AddDepthOrStencilStartBarrier(ImageBarrierBuilder&& barrierBuilder) noexcept
	{
		return AddDepthOrStencilStartBarrier(barrierBuilder);
	}

	// These functions can be used every frame.
	void SetDepthAttachment(
		size_t barrierIndex, const VKImageView& depthView, const VkClearDepthStencilValue& clearValue,
		VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOP
	) noexcept {
		m_renderingInfoBuilder.SetDepthAttachment(depthView, clearValue, loadOp, storeOP);

		m_startImageBarriers.SetImage(barrierIndex, depthView);
	}
	void SetDepthClearColour(const VkClearDepthStencilValue& clearColour) noexcept
	{
		m_renderingInfoBuilder.SetDepthClearColour(clearColour);
	}
	void SetDepthView(size_t barrierIndex, const VKImageView& depthView) noexcept
	{
		m_renderingInfoBuilder.SetDepthView(depthView);

		m_startImageBarriers.SetImage(barrierIndex, depthView);
	}

	void SetStencilAttachment(
		size_t barrierIndex, const VKImageView& stencilView, const VkClearDepthStencilValue& clearValue,
		VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOP
	) noexcept {
		m_renderingInfoBuilder.SetStencilAttachment(stencilView, clearValue, loadOp, storeOP);

		m_startImageBarriers.SetImage(barrierIndex, stencilView);
	}
	void SetStencilClearColour(const VkClearDepthStencilValue& clearColour) noexcept
	{
		m_renderingInfoBuilder.SetStencilClearColour(clearColour);
	}
	void SetStencilView(size_t barrierIndex, const VKImageView& stencilView) noexcept
	{
		m_renderingInfoBuilder.SetStencilView(stencilView);

		m_startImageBarriers.SetImage(barrierIndex, stencilView);
	}

	void SetColourView(
		size_t colourAttachmentIndex, size_t barrierIndex, const VKImageView& colourView
	) noexcept {
		m_renderingInfoBuilder.SetColourView(colourAttachmentIndex, colourView);

		m_startImageBarriers.SetImage(barrierIndex, colourView);
	}
	void SetColourClearValue(size_t colourAttachmentIndex, const VkClearColorValue& clearValue) noexcept
	{
		m_renderingInfoBuilder.SetColourClearValue(colourAttachmentIndex, clearValue);
	}

	void StartPass(const VKCommandBuffer& graphicsCmdBuffer, VkExtent2D renderArea) const noexcept
	{
		VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

		m_startImageBarriers.RecordBarriers(cmdBuffer);

		VkRenderingInfo renderingInfo = m_renderingInfoBuilder.BuildRenderingInfo(renderArea);

		vkCmdBeginRendering(cmdBuffer, &renderingInfo);
	}

	void EndPass(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
	{
		vkCmdEndRendering(graphicsCmdBuffer.Get());
	}

	void EndPassForSwapchain(
		const VKCommandBuffer& graphicsCmdBuffer, const VKImageView& srcColourView,
		const VKImageView& swapchainBackBuffer, const VkExtent3D& srcColourExtent
	) const noexcept {
		VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

		vkCmdEndRendering(cmdBuffer);

		VkImageBarrier2<2>{}
		.AddMemoryBarrier(
			ImageBarrierBuilder{}
			.Image(srcColourView)
			.StageMasks(
				VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT
			).AccessMasks(VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_2_TRANSFER_READ_BIT)
			.Layouts(VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		).AddMemoryBarrier(
			ImageBarrierBuilder{}
			.Image(swapchainBackBuffer)
			.StageMasks(
				VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT
			).AccessMasks(0u, VK_ACCESS_2_TRANSFER_WRITE_BIT)
			.Layouts(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		).RecordBarriers(cmdBuffer);

		// You can't copy textures inside of a render pass. Since the rendering won't have been
		// finished by then. So have to do it afterwards.
		graphicsCmdBuffer.Copy(
			srcColourView, swapchainBackBuffer,
			ImageCopyBuilder{}
			.Extent(srcColourExtent)
			.SrcImageAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
			.DstImageAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
		);

		VkImageBarrier2<>{}
		.AddMemoryBarrier(
			ImageBarrierBuilder{}
			.Image(swapchainBackBuffer)
			.StageMasks(
				VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
			).AccessMasks(VK_ACCESS_2_TRANSFER_WRITE_BIT, 0u)
			.Layouts(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		);
	}

private:
	[[nodiscard]]
	std::uint32_t AddStartImageBarrier(const ImageBarrierBuilder& barrierBuilder) noexcept
	{
		const std::uint32_t barrierIndex = m_startImageBarriers.GetCount();

		m_startImageBarriers.AddMemoryBarrier(barrierBuilder);

		return barrierIndex;
	}

private:
	RenderingInfoBuilder m_renderingInfoBuilder;
	VkImageBarrier2_1    m_startImageBarriers;

public:
	VkRenderPassManager(const VkRenderPassManager&) = delete;
	VkRenderPassManager& operator=(const VkRenderPassManager&) = delete;

	VkRenderPassManager(VkRenderPassManager&& other) noexcept
		: m_renderingInfoBuilder{ std::move(other.m_renderingInfoBuilder) },
		m_startImageBarriers{ std::move(other.m_startImageBarriers) }
	{}
	VkRenderPassManager& operator=(VkRenderPassManager&& other) noexcept
	{
		m_renderingInfoBuilder = std::move(other.m_renderingInfoBuilder);
		m_startImageBarriers   = std::move(other.m_startImageBarriers);

		return *this;
	}
};
#endif
