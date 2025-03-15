#ifndef VK_EXTERNAL_RENDER_PASS_HPP_
#define VK_EXTERNAL_RENDER_PASS_HPP_
#include <vector>
#include <limits>
#include <utility>
#include <ranges>
#include <algorithm>
#include <ExternalRenderPass.hpp>
#include <VkRenderPassManager.hpp>
#include <VkExternalResourceFactory.hpp>

class VkExternalRenderPass : public ExternalRenderPass
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
	VkExternalRenderPass(VkExternalResourceFactory* resourceFactory);

	void AddPipeline(std::uint32_t pipelineIndex) override;

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override;

	void RemovePipeline(std::uint32_t pipelineIndex) noexcept override;

	// Should be called after something like a window resize, where the buffer handles would change.
	void ResetAttachmentReferences() override;

	void SetDepthTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOp
	) override;
	// Only necessary if the LoadOP is clear.
	void SetDepthClearColour(float clearColour) override;

	void SetStencilTesting(
		std::uint32_t externalTextureIndex, ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOp
	) override;
	// Only necessary if the LoadOP is clear.
	void SetStencilClearColour(std::uint32_t clearColour) override;

	std::uint32_t AddRenderTarget(
		std::uint32_t externalTextureIndex,	ExternalAttachmentLoadOp loadOp,
		ExternalAttachmentStoreOp storeOp
	) override;
	// Only necessary if the LoadOP is clear.
	void SetRenderTargetClearColour(
		std::uint32_t renderTargetIndex, const DirectX::XMFLOAT4& clearColour
	) override;

	void SetSwapchainCopySource(std::uint32_t renderTargetIndex) noexcept override;

	void StartPass(const VKCommandBuffer& graphicsCmdBuffer, VkExtent2D renderArea) const noexcept;

	void EndPass(const VKCommandBuffer& graphicsCmdBuffer) const noexcept;

	void EndPassForSwapchain(
		const VKCommandBuffer& graphicsCmdBuffer, const VKImageView& swapchainBackBuffer
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

protected:
	VkExternalResourceFactory*     m_resourceFactory;
	VkRenderPassManager            m_renderPassManager;
	std::vector<PipelineDetails>   m_pipelineDetails;
	std::vector<AttachmentDetails> m_colourAttachmentDetails;
	AttachmentDetails              m_depthAttachmentDetails;
	AttachmentDetails              m_stencilAttachmentDetails;
	std::uint32_t                  m_swapchainCopySource;

public:
	VkExternalRenderPass(const VkExternalRenderPass&) = delete;
	VkExternalRenderPass& operator=(const VkExternalRenderPass&) = delete;

	VkExternalRenderPass(VkExternalRenderPass&& other) noexcept
		: m_resourceFactory{ std::exchange(other.m_resourceFactory, nullptr) },
		m_renderPassManager{ std::move(other.m_renderPassManager) },
		m_pipelineDetails{ std::move(other.m_pipelineDetails) },
		m_colourAttachmentDetails{ std::move(other.m_colourAttachmentDetails) },
		m_depthAttachmentDetails{ other.m_depthAttachmentDetails },
		m_stencilAttachmentDetails{ other.m_stencilAttachmentDetails },
		m_swapchainCopySource{ other.m_swapchainCopySource }
	{
		other.m_resourceFactory = nullptr;
	}
	VkExternalRenderPass& operator=(VkExternalRenderPass&& other) noexcept
	{
		m_resourceFactory          = std::exchange(other.m_resourceFactory, nullptr);
		m_renderPassManager        = std::move(other.m_renderPassManager);
		m_pipelineDetails          = std::move(other.m_pipelineDetails);
		m_colourAttachmentDetails  = std::move(other.m_colourAttachmentDetails);
		m_depthAttachmentDetails   = other.m_depthAttachmentDetails;
		m_stencilAttachmentDetails = other.m_stencilAttachmentDetails;
		m_swapchainCopySource      = other.m_swapchainCopySource;

		return *this;
	}
};

template<typename ModelManager_t>
class VkExternalRenderPassCommon : public VkExternalRenderPass
{
public:
	VkExternalRenderPassCommon(
		ModelManager_t* modelManager, VkExternalResourceFactory* resourceFactory
	) : VkExternalRenderPass{ resourceFactory }, m_modelManager{ modelManager }
	{}

	void AddModelBundle(std::uint32_t bundleIndex) override
	{
		for (PipelineDetails& pipelineDetails : m_pipelineDetails)
		{
			std::optional<size_t> oLocalIndex = m_modelManager->GetPipelineLocalIndex(
				bundleIndex, pipelineDetails.pipelineGlobalIndex
			);

			auto localIndex = std::numeric_limits<std::uint32_t>::max();

			if (!oLocalIndex)
				localIndex = m_modelManager->AddPipelineToModelBundle(
					bundleIndex, pipelineDetails.pipelineGlobalIndex
				);
			else
				localIndex = static_cast<std::uint32_t>(*oLocalIndex);

			std::vector<std::uint32_t>& modelIndices = pipelineDetails.modelBundleIndices;
			std::vector<std::uint32_t>& localIndices = pipelineDetails.pipelineLocalIndices;

			auto result = std::ranges::find(modelIndices, bundleIndex);

			size_t resultIndex = std::size(modelIndices);

			if (result == std::end(modelIndices))
			{
				modelIndices.emplace_back(0u);
				localIndices.emplace_back(0u);
			}
			else
				resultIndex = std::distance(std::begin(modelIndices), result);

			modelIndices[resultIndex] = bundleIndex;
			localIndices[resultIndex] = localIndex;
		}
	}

private:
	ModelManager_t* m_modelManager;

public:
	VkExternalRenderPassCommon(const VkExternalRenderPassCommon&) = delete;
	VkExternalRenderPassCommon& operator=(const VkExternalRenderPassCommon&) = delete;

	VkExternalRenderPassCommon(VkExternalRenderPassCommon&& other) noexcept
		: VkExternalRenderPass{ std::move(other) },
		m_modelManager{ std::exchange(other.m_modelManager, nullptr) }
	{}
	VkExternalRenderPassCommon& operator=(VkExternalRenderPassCommon&& other) noexcept
	{
		VkExternalRenderPass::operator=(std::move(other));
		m_modelManager = std::exchange(other.m_modelManager, nullptr);

		return *this;
	}
};
#endif
