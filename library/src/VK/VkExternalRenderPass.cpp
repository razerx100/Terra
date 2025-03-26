#include <VkExternalRenderPass.hpp>

VkExternalRenderPass::VkExternalRenderPass(VkExternalResourceFactory* resourceFactory)
	: m_resourceFactory{ resourceFactory }, m_renderPassManager{}, m_pipelineDetails{},
	m_colourAttachmentDetails{},
	m_depthAttachmentDetails
	{
		.textureIndex = std::numeric_limits<std::uint32_t>::max(),
		.barrierIndex = std::numeric_limits<std::uint32_t>::max()
	},
	m_stencilAttachmentDetails
	{
		.textureIndex = std::numeric_limits<std::uint32_t>::max(),
		.barrierIndex = std::numeric_limits<std::uint32_t>::max()
	}, m_swapchainCopySource{ std::numeric_limits<std::uint32_t>::max() }
{}

void VkExternalRenderPass::AddPipeline(std::uint32_t pipelineIndex)
{
	m_pipelineDetails.emplace_back(PipelineDetails{ .pipelineGlobalIndex = pipelineIndex });
}

void VkExternalRenderPass::RemoveModelBundle(std::uint32_t bundleIndex) noexcept
{
	for (size_t index = 0u; index < std::size(m_pipelineDetails); ++index)
	{
		PipelineDetails& pipelineDetails = m_pipelineDetails[index];

		std::vector<std::uint32_t>& bundleIndices        = pipelineDetails.modelBundleIndices;
		std::vector<std::uint32_t>& pipelineLocalIndices = pipelineDetails.pipelineLocalIndices;

		auto result = std::ranges::find(bundleIndices, bundleIndex);

		if (result != std::end(bundleIndices))
		{
			pipelineLocalIndices.erase(
				std::next(
					std::begin(pipelineLocalIndices), std::distance(std::begin(bundleIndices), result)
				)
			);

			bundleIndices.erase(result);
		}
	}
}

void VkExternalRenderPass::RemovePipeline(std::uint32_t pipelineIndex) noexcept
{
	auto result = std::ranges::find(
		m_pipelineDetails, pipelineIndex,
		[](const PipelineDetails& indexDetails)
		{
			return indexDetails.pipelineGlobalIndex;
		}
	);

	if (result != std::end(m_pipelineDetails))
		m_pipelineDetails.erase(result);
}

VkAttachmentStoreOp VkExternalRenderPass::GetVkStoreOp(ExternalAttachmentStoreOp storeOp) noexcept
{
	VkAttachmentStoreOp vkStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	if (storeOp == ExternalAttachmentStoreOp::Store)
		vkStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

	return vkStoreOp;
}

VkAttachmentLoadOp VkExternalRenderPass::GetVkLoadOp(ExternalAttachmentLoadOp loadOp) noexcept
{
	VkAttachmentLoadOp vkLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

	if (loadOp == ExternalAttachmentLoadOp::Load)
		vkLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	else if (loadOp == ExternalAttachmentLoadOp::Clear)
		vkLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

	return vkLoadOp;
}

void VkExternalRenderPass::ResetAttachmentReferences()
{
	if (m_depthAttachmentDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		VkExternalTexture* externalTexture = m_resourceFactory->GetVkExternalTexture(
			m_depthAttachmentDetails.textureIndex
		);

		m_renderPassManager.SetDepthView(
			m_depthAttachmentDetails.barrierIndex, externalTexture->GetTextureView().GetView()
		);
	}

	if (m_stencilAttachmentDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		VkExternalTexture* externalTexture = m_resourceFactory->GetVkExternalTexture(
			m_stencilAttachmentDetails.textureIndex
		);

		m_renderPassManager.SetStencilView(
			m_stencilAttachmentDetails.barrierIndex, externalTexture->GetTextureView().GetView()
		);
	}

	const size_t colourAttachmentCount = std::size(m_colourAttachmentDetails);

	for (size_t index = 0u; index < colourAttachmentCount; ++index)
	{
		const AttachmentDetails& colourAttachmentDetails = m_colourAttachmentDetails[index];

		VkExternalTexture* externalTexture = m_resourceFactory->GetVkExternalTexture(
			colourAttachmentDetails.textureIndex
		);

		m_renderPassManager.SetColourView(
			index, colourAttachmentDetails.barrierIndex, externalTexture->GetTextureView().GetView()
		);
	}
}

void VkExternalRenderPass::SetDepthTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp, ExternalAttachmentStoreOp storeOp
) {
	VkAccessFlagBits newAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	const VkAttachmentLoadOp vkLoadOp   = GetVkLoadOp(loadOp);
	const VkAttachmentStoreOp vkStoreOp = GetVkStoreOp(storeOp);

	if (loadOp == ExternalAttachmentLoadOp::Clear || storeOp == ExternalAttachmentStoreOp::Store)
		newAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkExternalTexture* externalTexture = m_resourceFactory->GetVkExternalTexture(externalTextureIndex);

	m_depthAttachmentDetails.textureIndex = externalTextureIndex;

	// Set the previous stage to Early fragment if it is the first use as that's why depth/stencil
	// testing starts.
	if (externalTexture->GetCurrentPipelineStage() == VK_PIPELINE_STAGE_NONE)
		externalTexture->SetCurrentPipelineStage(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

	std::uint32_t& depthBarrierIndex      = m_depthAttachmentDetails.barrierIndex;

	depthBarrierIndex = m_renderPassManager.AddStartImageBarrier(
		externalTexture->TransitionState(
			newAccessFlag, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		)
	);

	VkClearDepthStencilValue clearValue{ .depth = 1.f };

	m_renderPassManager.SetDepthAttachment(
		depthBarrierIndex, externalTexture->GetTextureView().GetView(), clearValue, vkLoadOp, vkStoreOp
	);
}

void VkExternalRenderPass::SetDepthClearColour(float clearColour)
{
	m_renderPassManager.SetDepthClearColour(VkClearDepthStencilValue{ .depth = clearColour });
}

void VkExternalRenderPass::SetStencilTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp, ExternalAttachmentStoreOp storeOp
) {
	VkAccessFlagBits newAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	const VkAttachmentLoadOp vkLoadOp   = GetVkLoadOp(loadOp);
	const VkAttachmentStoreOp vkStoreOp = GetVkStoreOp(storeOp);

	if (loadOp == ExternalAttachmentLoadOp::Clear || storeOp == ExternalAttachmentStoreOp::Store)
		newAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkExternalTexture* externalTexture = m_resourceFactory->GetVkExternalTexture(externalTextureIndex);

	m_stencilAttachmentDetails.textureIndex = externalTextureIndex;

	// Set the previous stage to Early fragment if it is the first use as that's why depth/stencil
	// testing starts.
	if (externalTexture->GetCurrentPipelineStage() == VK_PIPELINE_STAGE_NONE)
		externalTexture->SetCurrentPipelineStage(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

	std::uint32_t& stencilBarrierIndex      = m_stencilAttachmentDetails.barrierIndex;

	stencilBarrierIndex = m_renderPassManager.AddStartImageBarrier(
		externalTexture->TransitionState(
			newAccessFlag, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		)
	);

	VkClearDepthStencilValue clearValue{ .stencil = 0u };

	m_renderPassManager.SetStencilAttachment(
		stencilBarrierIndex, externalTexture->GetTextureView().GetView(), clearValue, vkLoadOp, vkStoreOp
	);
}

void VkExternalRenderPass::SetStencilClearColour(std::uint32_t clearColour)
{
	m_renderPassManager.SetStencilClearColour(VkClearDepthStencilValue{ .stencil = clearColour });
}

std::uint32_t VkExternalRenderPass::AddRenderTarget(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp, ExternalAttachmentStoreOp storeOp
) {
	VkAccessFlagBits newAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

	const VkAttachmentLoadOp vkLoadOp   = GetVkLoadOp(loadOp);
	const VkAttachmentStoreOp vkStoreOp = GetVkStoreOp(storeOp);

	if (loadOp == ExternalAttachmentLoadOp::Clear || storeOp == ExternalAttachmentStoreOp::Store)
		newAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkExternalTexture* externalTexture = m_resourceFactory->GetVkExternalTexture(externalTextureIndex);

	// Set the previous stage to COLOUR ATTACHMENT OUTPUT if it is the first use as the top of
	// the pipeline bit doesn't queue a wait operation
	if (externalTexture->GetCurrentPipelineStage() == VK_PIPELINE_STAGE_NONE)
		externalTexture->SetCurrentPipelineStage(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	const std::uint32_t colourBarrierIndex = m_renderPassManager.AddStartImageBarrier(
		externalTexture->TransitionState(
			newAccessFlag, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		)
	);

	const auto renderTargetIndex = static_cast<std::uint32_t>(std::size(m_colourAttachmentDetails));

	m_colourAttachmentDetails.emplace_back(
		AttachmentDetails{ .textureIndex = externalTextureIndex, .barrierIndex = colourBarrierIndex }
	);

	const VkClearColorValue clearValue{ .float32{ 0.f, 0.f, 0.f, 0.f }};

	m_renderPassManager.AddColourAttachment(
		externalTexture->GetTextureView().GetView(), clearValue, vkLoadOp, vkStoreOp
	);

	return renderTargetIndex;
}

void VkExternalRenderPass::SetRenderTargetClearColour(
	std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
) {
	const VkClearColorValue clearValue
	{
		.float32{ clearColour.x, clearColour.y, clearColour.z, clearColour.w }
	};

	m_renderPassManager.SetColourClearValue(renderTargetIndex, clearValue);
}

void VkExternalRenderPass::SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept
{
	m_swapchainCopySource = m_colourAttachmentDetails[renderTargetIndex].textureIndex;
}

void VkExternalRenderPass::StartPass(
	const VKCommandBuffer& graphicsCmdBuffer, VkExtent2D renderArea
) const noexcept {
	m_renderPassManager.StartPass(graphicsCmdBuffer, renderArea);
}

void VkExternalRenderPass::EndPass(const VKCommandBuffer& graphicsCmdBuffer) const noexcept
{
	m_renderPassManager.EndPass(graphicsCmdBuffer);
}

void VkExternalRenderPass::EndPassForSwapchain(
	const VKCommandBuffer& graphicsCmdBuffer, const VKImageView& swapchainBackBuffer
) const noexcept {
	const VkTextureView& srcTextureView = m_resourceFactory->GetVkTextureView(m_swapchainCopySource);

	const VkExtent3D srcColourExtent    = srcTextureView.GetTexture().GetExtent();

	m_renderPassManager.EndPassForSwapchain(
		graphicsCmdBuffer, srcTextureView.GetView(), swapchainBackBuffer, srcColourExtent
	);
}
