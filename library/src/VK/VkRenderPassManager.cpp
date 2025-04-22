#include <VkRenderPassManager.hpp>

namespace Terra
{
void VkRenderPassManager::AddColourAttachment(
	const VKImageView& colourView, const VkClearColorValue& clearValue, VkAttachmentLoadOp loadOp,
	VkAttachmentStoreOp storeOP
) noexcept {
	m_renderingInfoBuilder.AddColourAttachment(colourView, clearValue, loadOp, storeOP);
}

void VkRenderPassManager::SetBarrierImageView(
	std::uint32_t barrierIndex, const VKImageView& imageView
) noexcept {
	if (barrierIndex != std::numeric_limits<std::uint32_t>::max())
		m_startImageBarriers.SetImage(barrierIndex, imageView);
}

void VkRenderPassManager::SetSrcStage(size_t barrierIndex, VkPipelineStageFlagBits2 srcStageFlag) noexcept
{
	if (barrierIndex != std::numeric_limits<std::uint32_t>::max())
		m_startImageBarriers.SetSrcStage(barrierIndex, srcStageFlag);
}

void VkRenderPassManager::SetDepthAttachment(
	std::uint32_t barrierIndex, const VKImageView& depthView, const VkClearDepthStencilValue& clearValue,
	VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOP
) noexcept {
	m_renderingInfoBuilder.SetDepthAttachment(depthView, clearValue, loadOp, storeOP);

	SetBarrierImageView(barrierIndex, depthView);
}

void VkRenderPassManager::SetDepthClearColour(const VkClearDepthStencilValue& clearColour) noexcept
{
	m_renderingInfoBuilder.SetDepthClearColour(clearColour);
}

void VkRenderPassManager::SetDepthView(std::uint32_t barrierIndex, const VKImageView& depthView) noexcept
{
	m_renderingInfoBuilder.SetDepthView(depthView);

	SetBarrierImageView(barrierIndex, depthView);
}

void VkRenderPassManager::SetStencilAttachment(
	std::uint32_t barrierIndex, const VKImageView& stencilView, const VkClearDepthStencilValue& clearValue,
	VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOP
) noexcept {
	m_renderingInfoBuilder.SetStencilAttachment(stencilView, clearValue, loadOp, storeOP);

	SetBarrierImageView(barrierIndex, stencilView);
}

void VkRenderPassManager::SetStencilClearColour(const VkClearDepthStencilValue& clearColour) noexcept
{
	m_renderingInfoBuilder.SetStencilClearColour(clearColour);
}

void VkRenderPassManager::SetStencilView(
	std::uint32_t barrierIndex, const VKImageView& stencilView
) noexcept {
	m_renderingInfoBuilder.SetStencilView(stencilView);

	SetBarrierImageView(barrierIndex, stencilView);
}

void VkRenderPassManager::SetColourView(
	size_t colourAttachmentIndex, std::uint32_t barrierIndex, const VKImageView& colourView
) noexcept {
	m_renderingInfoBuilder.SetColourView(colourAttachmentIndex, colourView);

	SetBarrierImageView(barrierIndex, colourView);
}

void VkRenderPassManager::SetColourClearValue(
	size_t colourAttachmentIndex, const VkClearColorValue& clearValue
) noexcept {
	m_renderingInfoBuilder.SetColourClearValue(colourAttachmentIndex, clearValue);
}

void VkRenderPassManager::StartPass(const VKCommandBuffer& graphicsCmdBuffer, VkExtent2D renderArea) const noexcept
{
	VkCommandBuffer cmdBuffer = graphicsCmdBuffer.Get();

	if (m_startImageBarriers.GetCount())
		m_startImageBarriers.RecordBarriers(cmdBuffer);

	VkRenderingInfo renderingInfo = m_renderingInfoBuilder.BuildRenderingInfo(renderArea);

	vkCmdBeginRendering(cmdBuffer, &renderingInfo);
}

void VkRenderPassManager::EndPass(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
{
	vkCmdEndRendering(graphicsCmdBuffer.Get());
}

void VkRenderPassManager::EndPassForSwapchain(
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
		.Layouts(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
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
	).RecordBarriers(cmdBuffer);
}

std::uint32_t VkRenderPassManager::AddStartImageBarrier(
	const ImageBarrierBuilder& barrierBuilder
) noexcept {
	auto barrierIndex = std::numeric_limits<std::uint32_t>::max();

	const auto isBarrierRequired = [](const ImageBarrierBuilder& barrierBuilder) -> bool
		{
			const VkImageMemoryBarrier2& imageBarrier = barrierBuilder.Get();

			const bool isAccessSame = imageBarrier.srcAccessMask == imageBarrier.dstAccessMask;
			const bool isLayoutSame = imageBarrier.oldLayout == imageBarrier.newLayout;

			return !isLayoutSame || !isAccessSame;
		}(barrierBuilder);

	if (isBarrierRequired)
	{
		barrierIndex = m_startImageBarriers.GetCount();

		m_startImageBarriers.AddMemoryBarrier(barrierBuilder);
	}

	return barrierIndex;
}
}
