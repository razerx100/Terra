#ifndef RENDER_ENGINE_MS_HPP_
#define RENDER_ENGINE_MS_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineMSDeviceExtension : public RenderEngineDeviceExtension
{
public:
	void SetDeviceExtensions(VkDeviceExtensionManager& extensionManager) noexcept override;
};

class RenderEngineMS : public
	RenderEngineCommon
	<
		ModelManagerMS,
		MeshManagerMS,
		GraphicsPipelineMS,
		RenderEngineMS
	>
{
	friend class RenderEngineCommon
		<
			ModelManagerMS,
			MeshManagerMS,
			GraphicsPipelineMS,
			RenderEngineMS
		>;

public:
	RenderEngineMS(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	void FinaliseInitialisation() override;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) override;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) override;

private:
	[[nodiscard]]
	VkSemaphore ExecutePipelineStages(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);

	[[nodiscard]]
	VkSemaphore GenericTransferStage(
		size_t frameIndex, std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);
	[[nodiscard]]
	VkSemaphore DrawingStage(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, VkSemaphore waitSemaphore
	);

	void SetGraphicsDescriptorBufferLayout();
	void SetModelGraphicsDescriptors();

	void _updatePerFrame(VkDeviceSize frameIndex) const noexcept
	{
		m_modelBuffers->Update(frameIndex);
	}

private:
	void DrawRenderPassPipelines(
		const VKCommandBuffer& graphicsCmdBuffer, const ExternalRenderPass_t& renderPass
	) const noexcept;

public:
	RenderEngineMS(const RenderEngineMS&) = delete;
	RenderEngineMS& operator=(const RenderEngineMS&) = delete;

	RenderEngineMS(RenderEngineMS&& other) noexcept
		: RenderEngineCommon{ std::move(other) }
	{}
	RenderEngineMS& operator=(RenderEngineMS&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));

		return *this;
	}
};
#endif
