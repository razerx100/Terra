#ifndef RENDER_ENGINE_MESH_SHADER_HPP_
#define RENDER_ENGINE_MESH_SHADER_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineMSDeviceExtension : public RenderEngineDeviceExtension
{
public:
	void SetDeviceExtensions(VkDeviceExtensionManager& extensionManager) noexcept override;
};

class RenderEngineMS : public RenderEngineCommon<ModelManagerMS, RenderEngineMS>
{
	friend class RenderEngineCommon<ModelManagerMS, RenderEngineMS>;

public:
	RenderEngineMS(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundleMS>&& modelBundle, const ShaderName& fragmentShader
	) override;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle) override;

private:
	[[nodiscard]]
	static ModelManagerMS GetModelManager(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

	[[nodiscard]]
	VkSemaphore GenericTransferStage(
		size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
		std::uint64_t semaphoreCounter, VkSemaphore waitSemaphore
	);
	[[nodiscard]]
	VkSemaphore DrawingStage(
		size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
		std::uint64_t semaphoreCounter, VkSemaphore waitSemaphore
	);

	void SetupPipelineStages();

private:
	// Graphics
	static constexpr std::uint32_t s_cameraBindingSlot = 5u;

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
