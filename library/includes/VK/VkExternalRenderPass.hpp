#ifndef VK_EXTERNAL_RENDER_PASS_HPP_
#define VK_EXTERNAL_RENDER_PASS_HPP_
#include <bitset>
#include <vector>
#include <limits>
#include <utility>
#include <ranges>
#include <algorithm>
#include <ExternalRenderPass.hpp>
#include <VkRenderPassManager.hpp>
#include <VkExternalResourceFactory.hpp>

namespace Terra
{
class VkExternalRenderPass
{
	struct AttachmentDetails
	{
		std::uint32_t textureIndex = std::numeric_limits<std::uint32_t>::max();
		std::uint32_t barrierIndex = std::numeric_limits<std::uint32_t>::max();
	};

public:
	struct PipelineDetails
	{
		std::uint32_t              pipelineGlobalIndex;
		std::vector<std::uint32_t> modelBundleIndices;
		std::vector<std::uint32_t> pipelineLocalIndices;
	};

public:
	VkExternalRenderPass();

	void AddPipeline(std::uint32_t pipelineIndex);

	template<class ModelManager_t>
	void AddLocalPipelinesOfModelBundle(
		std::uint32_t modelBundleIndex, const ModelManager_t& modelManager
	) {
		for (PipelineDetails& pipelineDetails : m_pipelineDetails)
		{
			std::optional<size_t> oLocalIndex = modelManager.GetPipelineLocalIndex(
				modelBundleIndex, pipelineDetails.pipelineGlobalIndex
			);

			// I guess it should be okay for a model bundle to not have all the Pipelines?
			if (!oLocalIndex)
				continue;

			auto localIndex = static_cast<std::uint32_t>(*oLocalIndex);

			std::vector<std::uint32_t>& modelBundleIndices = pipelineDetails.modelBundleIndices;
			std::vector<std::uint32_t>& pipelineIndicesInBundle
				= pipelineDetails.pipelineLocalIndices;

			auto result = std::ranges::find(modelBundleIndices, modelBundleIndex);

			size_t resultIndex = std::size(modelBundleIndices);

			if (result == std::end(modelBundleIndices))
			{
				modelBundleIndices.emplace_back(0u);
				pipelineIndicesInBundle.emplace_back(0u);
			}
			else
				resultIndex = std::distance(std::begin(modelBundleIndices), result);

			modelBundleIndices[resultIndex]      = modelBundleIndex;
			pipelineIndicesInBundle[resultIndex] = localIndex;
		}
	}

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept;

	void RemovePipeline(std::uint32_t pipelineIndex) noexcept;

	// Should be called after something like a window resize, where the buffer handles would change.
	void ResetAttachmentReferences(VkExternalResourceFactory& resourceFactory);

	[[nodiscard]]
	std::uint32_t AddStartBarrier(
		std::uint32_t externalTextureIndex, ExternalTextureTransition transitionState,
		VkExternalResourceFactory& resourceFactory
	) noexcept;
	void UpdateStartBarrierResource(
		std::uint32_t barrierIndex, std::uint32_t externalTextureIndex,
		VkExternalResourceFactory& resourceFactory
	) noexcept;

	void SetDepthTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOp, VkExternalResourceFactory& resourceFactory
	);
	// Only necessary if the LoadOP is clear.
	void SetDepthClearColour(float clearColour);

	void SetStencilTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOp, VkExternalResourceFactory& resourceFactory
	);
	// Only necessary if the LoadOP is clear.
	void SetStencilClearColour(std::uint32_t clearColour);

	std::uint32_t AddRenderTarget(
		std::uint32_t externalTextureIndex,	ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOp, VkExternalResourceFactory& resourceFactory
	);
	// Only necessary if the LoadOP is clear.
	void SetRenderTargetClearColour(
		std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
	);

	void SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept;

	void StartPass(const VKCommandBuffer& graphicsCmdBuffer, VkExtent2D renderArea) const noexcept;

	void EndPass(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

	void EndPassForSwapchain(
		const VKCommandBuffer& graphicsCmdBuffer, const VKImageView& swapchainBackBuffer,
		VkExternalResourceFactory& resourceFactory
	) const noexcept;

	[[nodiscard]]
	const std::vector<PipelineDetails>& GetPipelineDetails() const noexcept
	{
		return m_pipelineDetails;
	}

private:
	[[nodiscard]]
	static VkAttachmentStoreOp GetVkStoreOp(ExternalAttachmentStoreOp storeOp) noexcept;

	[[nodiscard]]
	static VkAttachmentLoadOp GetVkLoadOp(ExternalAttachmentLoadOp loadOp) noexcept;

	static constexpr size_t s_maxAttachmentCount = 10u;

private:
	VkRenderPassManager               m_renderPassManager;
	std::vector<PipelineDetails>      m_pipelineDetails;
	std::vector<AttachmentDetails>    m_colourAttachmentDetails;
	AttachmentDetails                 m_depthAttachmentDetails;
	AttachmentDetails                 m_stencilAttachmentDetails;
	std::uint32_t                     m_swapchainCopySource;
	std::bitset<s_maxAttachmentCount> m_firstUseFlags;

	static constexpr size_t s_depthAttachmentIndex   = 8u;
	static constexpr size_t s_stencilAttachmentIndex = 9u;

public:
	VkExternalRenderPass(const VkExternalRenderPass&) = delete;
	VkExternalRenderPass& operator=(const VkExternalRenderPass&) = delete;

	VkExternalRenderPass(VkExternalRenderPass&& other) noexcept
		: m_renderPassManager{ std::move(other.m_renderPassManager) },
		m_pipelineDetails{ std::move(other.m_pipelineDetails) },
		m_colourAttachmentDetails{ std::move(other.m_colourAttachmentDetails) },
		m_depthAttachmentDetails{ other.m_depthAttachmentDetails },
		m_stencilAttachmentDetails{ other.m_stencilAttachmentDetails },
		m_swapchainCopySource{ other.m_swapchainCopySource },
		m_firstUseFlags{ other.m_firstUseFlags }
	{}
	VkExternalRenderPass& operator=(VkExternalRenderPass&& other) noexcept
	{
		m_renderPassManager        = std::move(other.m_renderPassManager);
		m_pipelineDetails          = std::move(other.m_pipelineDetails);
		m_colourAttachmentDetails  = std::move(other.m_colourAttachmentDetails);
		m_depthAttachmentDetails   = other.m_depthAttachmentDetails;
		m_stencilAttachmentDetails = other.m_stencilAttachmentDetails;
		m_swapchainCopySource      = other.m_swapchainCopySource;
		m_firstUseFlags            = other.m_firstUseFlags;

		return *this;
	}
};
}
#endif
