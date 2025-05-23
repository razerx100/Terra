#ifndef VK_RENDER_ENGINE_MS_HPP_
#define VK_RENDER_ENGINE_MS_HPP_
#include <VkRenderEngine.hpp>
#include <VkModelManager.hpp>

namespace Terra
{
namespace RenderEngineMSDeviceExtension
{
	void SetDeviceExtensions(VkDeviceExtensionManager& extensionManager) noexcept;
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
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	);

	void FinaliseInitialisation();

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle);

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddMeshBundle(MeshBundleTemporaryData&& meshBundle);

	void SetShaderPath(const std::wstring& shaderPath)
	{
		_setShaderPath(shaderPath);
	}

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
		m_modelBuffers.Update(frameIndex);
	}

	[[nodiscard]]
	static std::vector<std::uint32_t> GetModelBuffersQueueFamilies(
		[[maybe_unused]] const VkDeviceManager& deviceManager
	) noexcept {
		return {};
	}

	[[nodiscard]]
	static ModelManagerMS CreateModelManager(
		[[maybe_unused]] const VkDeviceManager& deviceManager,
		[[maybe_unused]] MemoryManager* memoryManager,
		[[maybe_unused]] size_t frameCount
	) {
		return ModelManagerMS{};
	}

private:
	void DrawRenderPassPipelines(
		const VKCommandBuffer& graphicsCmdBuffer, const VkExternalRenderPass& renderPass
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
}
#endif
