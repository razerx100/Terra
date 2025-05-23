#include <array>
#include <VkExternalRenderPass.hpp>

namespace Terra
{
struct TransitionData
{
	VkAccessFlagBits        access;
	VkImageLayout           layout;
	VkPipelineStageFlagBits pipelineStage;
};

static constexpr std::array s_externalTextureTransitionMap
{
	TransitionData
	{
		.access        = VK_ACCESS_SHADER_READ_BIT,
		.layout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.pipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
	} // FragmentShaderReadOnly
};

// VK External Render Pass
VkExternalRenderPass::VkExternalRenderPass()
	: m_renderPassManager{}, m_pipelineDetails{}, m_colourAttachmentDetails{},
	m_depthAttachmentDetails
	{
		.textureIndex = std::numeric_limits<std::uint32_t>::max(),
		.barrierIndex = std::numeric_limits<std::uint32_t>::max()
	},
	m_stencilAttachmentDetails
	{
		.textureIndex = std::numeric_limits<std::uint32_t>::max(),
		.barrierIndex = std::numeric_limits<std::uint32_t>::max()
	}, m_swapchainCopySource{ std::numeric_limits<std::uint32_t>::max() },
	m_firstUseFlags{ 0u }
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
					std::begin(pipelineLocalIndices),
					std::distance(std::begin(bundleIndices), result)
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

void VkExternalRenderPass::ResetAttachmentReferences(VkExternalResourceFactory& resourceFactory)
{
	if (m_depthAttachmentDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		VkExternalTexture* externalTexture = resourceFactory.GetExternalTextureRP(
			m_depthAttachmentDetails.textureIndex
		);

		m_renderPassManager.SetDepthView(
			m_depthAttachmentDetails.barrierIndex, externalTexture->GetTextureView().GetView()
		);

		if (m_firstUseFlags[s_depthAttachmentIndex])
			m_renderPassManager.SetSrcStage(
				m_depthAttachmentDetails.barrierIndex, externalTexture->GetCurrentPipelineStage()
			);
	}

	if (m_stencilAttachmentDetails.textureIndex != std::numeric_limits<std::uint32_t>::max())
	{
		VkExternalTexture* externalTexture = resourceFactory.GetExternalTextureRP(
			m_stencilAttachmentDetails.textureIndex
		);

		m_renderPassManager.SetStencilView(
			m_stencilAttachmentDetails.barrierIndex, externalTexture->GetTextureView().GetView()
		);

		if (m_firstUseFlags[s_stencilAttachmentIndex])
			m_renderPassManager.SetSrcStage(
				m_stencilAttachmentDetails.barrierIndex, externalTexture->GetCurrentPipelineStage()
			);
	}

	const size_t colourAttachmentCount = std::size(m_colourAttachmentDetails);

	for (size_t index = 0u; index < colourAttachmentCount; ++index)
	{
		const AttachmentDetails& colourAttachmentDetails = m_colourAttachmentDetails[index];

		VkExternalTexture* externalTexture = resourceFactory.GetExternalTextureRP(
			colourAttachmentDetails.textureIndex
		);

		m_renderPassManager.SetColourView(
			index, colourAttachmentDetails.barrierIndex,
			externalTexture->GetTextureView().GetView()
		);

		if (m_firstUseFlags[index])
			m_renderPassManager.SetSrcStage(
				colourAttachmentDetails.barrierIndex, externalTexture->GetCurrentPipelineStage()
			);
	}
}

std::uint32_t VkExternalRenderPass::AddStartBarrier(
	std::uint32_t externalTextureIndex, ExternalTextureTransition transitionState,
	VkExternalResourceFactory& resourceFactory
) noexcept {
	const TransitionData transitionData
		= s_externalTextureTransitionMap[static_cast<size_t>(transitionState)];

	VkExternalTexture* externalTexture = resourceFactory.GetExternalTextureRP(
		externalTextureIndex
	);

	return m_renderPassManager.AddStartImageBarrier(
		externalTexture->TransitionState(
			transitionData.access, transitionData.layout, transitionData.pipelineStage
		)
	);
}

void VkExternalRenderPass::UpdateStartBarrierResource(
	std::uint32_t barrierIndex, std::uint32_t externalTextureIndex,
	VkExternalResourceFactory& resourceFactory
) noexcept {
	VkExternalTexture* externalTexture = resourceFactory.GetExternalTextureRP(
		externalTextureIndex
	);

	m_renderPassManager.SetBarrierImageView(
		barrierIndex, externalTexture->GetTextureView().GetView()
	);
}

void VkExternalRenderPass::SetDepthTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOp, VkExternalResourceFactory& resourceFactory
) {
	VkAccessFlagBits newAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	const VkAttachmentLoadOp vkLoadOp   = GetVkLoadOp(loadOp);
	const VkAttachmentStoreOp vkStoreOp = GetVkStoreOp(storeOp);

	if (loadOp == ExternalAttachmentLoadOp::Clear || storeOp == ExternalAttachmentStoreOp::Store)
		newAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkExternalTexture* externalTexture = resourceFactory.GetExternalTextureRP(
		externalTextureIndex
	);

	m_depthAttachmentDetails.textureIndex = externalTextureIndex;

	m_firstUseFlags[s_depthAttachmentIndex]
		= externalTexture->GetCurrentPipelineStage() == VK_PIPELINE_STAGE_NONE;

	// Set the previous stage to Early fragment if it is the first use as that's why depth/stencil
	// testing starts.
	if (m_firstUseFlags[s_depthAttachmentIndex])
		externalTexture->SetCurrentPipelineStage(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

	std::uint32_t& depthBarrierIndex = m_depthAttachmentDetails.barrierIndex;

	depthBarrierIndex = m_renderPassManager.AddStartImageBarrier(
		externalTexture->TransitionState(
			newAccessFlag, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		)
	);

	VkClearDepthStencilValue clearValue{ .depth = 1.f };

	m_renderPassManager.SetDepthAttachment(
		depthBarrierIndex, externalTexture->GetTextureView().GetView(), clearValue, vkLoadOp,
		vkStoreOp
	);
}

void VkExternalRenderPass::SetDepthClearColour(
	float clearColour, [[maybe_unused]] VkExternalResourceFactory& resourceFactory
) {
	m_renderPassManager.SetDepthClearColour(VkClearDepthStencilValue{ .depth = clearColour });
}

void VkExternalRenderPass::SetStencilTesting(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOp, VkExternalResourceFactory& resourceFactory
) {
	VkAccessFlagBits newAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	const VkAttachmentLoadOp vkLoadOp   = GetVkLoadOp(loadOp);
	const VkAttachmentStoreOp vkStoreOp = GetVkStoreOp(storeOp);

	if (loadOp == ExternalAttachmentLoadOp::Clear || storeOp == ExternalAttachmentStoreOp::Store)
		newAccessFlag = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkExternalTexture* externalTexture = resourceFactory.GetExternalTextureRP(
		externalTextureIndex
	);

	m_stencilAttachmentDetails.textureIndex = externalTextureIndex;

	m_firstUseFlags[s_stencilAttachmentIndex]
		= externalTexture->GetCurrentPipelineStage() == VK_PIPELINE_STAGE_NONE;

	// Set the previous stage to Early fragment if it is the first use as that's why depth/stencil
	// testing starts.
	if (m_firstUseFlags[s_stencilAttachmentIndex])
		externalTexture->SetCurrentPipelineStage(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

	std::uint32_t& stencilBarrierIndex = m_stencilAttachmentDetails.barrierIndex;

	stencilBarrierIndex = m_renderPassManager.AddStartImageBarrier(
		externalTexture->TransitionState(
			newAccessFlag, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		)
	);

	VkClearDepthStencilValue clearValue{ .stencil = 0u };

	m_renderPassManager.SetStencilAttachment(
		stencilBarrierIndex, externalTexture->GetTextureView().GetView(), clearValue, vkLoadOp,
		vkStoreOp
	);
}

void VkExternalRenderPass::SetStencilClearColour(
	std::uint32_t clearColour, [[maybe_unused]] VkExternalResourceFactory& resourceFactory
) {
	m_renderPassManager.SetStencilClearColour(VkClearDepthStencilValue{ .stencil = clearColour });
}

std::uint32_t VkExternalRenderPass::AddRenderTarget(
	std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
	ExternalAttachmentStoreOp storeOp, VkExternalResourceFactory& resourceFactory
) {
	VkAccessFlagBits newAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

	const VkAttachmentLoadOp vkLoadOp   = GetVkLoadOp(loadOp);
	const VkAttachmentStoreOp vkStoreOp = GetVkStoreOp(storeOp);

	if (loadOp == ExternalAttachmentLoadOp::Clear || storeOp == ExternalAttachmentStoreOp::Store)
		newAccessFlag = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkExternalTexture* externalTexture = resourceFactory.GetExternalTextureRP(
		externalTextureIndex
	);

	const size_t renderTargetIndex     = std::size(m_colourAttachmentDetails);

	m_firstUseFlags[renderTargetIndex]
		= externalTexture->GetCurrentPipelineStage() == VK_PIPELINE_STAGE_NONE;

	// Set the previous stage to COLOUR ATTACHMENT OUTPUT if it is the first use as the top of
	// the pipeline bit doesn't queue a wait operation
	if (m_firstUseFlags[renderTargetIndex])
		externalTexture->SetCurrentPipelineStage(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	const std::uint32_t colourBarrierIndex = m_renderPassManager.AddStartImageBarrier(
		externalTexture->TransitionState(
			newAccessFlag, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		)
	);

	const auto u32RenderTargetIndex = static_cast<std::uint32_t>(renderTargetIndex);

	m_colourAttachmentDetails.emplace_back(
		AttachmentDetails{
			.textureIndex = externalTextureIndex, .barrierIndex = colourBarrierIndex
		}
	);

	const VkClearColorValue clearValue{ .float32{ 0.f, 0.f, 0.f, 0.f }};

	m_renderPassManager.AddColourAttachment(
		externalTexture->GetTextureView().GetView(), clearValue, vkLoadOp, vkStoreOp
	);

	return u32RenderTargetIndex;
}

void VkExternalRenderPass::SetRenderTargetClearColour(
	std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour,
	[[maybe_unused]] VkExternalResourceFactory& resourceFactory
) {
	const VkClearColorValue clearValue
	{
		.float32{ clearColour.x, clearColour.y, clearColour.z, clearColour.w }
	};

	m_renderPassManager.SetColourClearValue(renderTargetIndex, clearValue);
}

void VkExternalRenderPass::SetSwapchainCopySource(
	std::uint32_t renderTargetIndex, [[maybe_unused]] VkExternalResourceFactory& resourceFactory
) noexcept {
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
	const VKCommandBuffer& graphicsCmdBuffer, const VKImageView& swapchainBackBuffer,
	VkExternalResourceFactory& resourceFactory
) const noexcept {
	const VkTextureView& srcTextureView = resourceFactory.GetVkTextureView(
		m_swapchainCopySource
	);

	const VkExtent3D srcColourExtent    = srcTextureView.GetTexture().GetExtent();

	m_renderPassManager.EndPassForSwapchain(
		graphicsCmdBuffer, srcTextureView.GetView(), swapchainBackBuffer, srcColourExtent
	);
}
}
