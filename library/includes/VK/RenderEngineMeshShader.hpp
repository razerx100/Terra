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
	std::uint32_t AddModel(
		std::shared_ptr<ModelMS>&& model, const ShaderName& fragmentShader
	) override;
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const ShaderName& fragmentShader
	) override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle) override;

	void Render(
		size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
		std::uint64_t frameNumber
	) override;

private:
	[[nodiscard]]
	static ModelManagerMS GetModelManager(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

private:
	// These might not work. As a binding slot with variable descriptor count must be at the end.
	// But lets see.
	static constexpr std::uint32_t s_cameraBindingSlot          = 6u;
	static constexpr std::uint32_t s_materialBindingSlot        = 7u;
	static constexpr std::uint32_t s_combinedTextureBindingSlot = 8u;
	static constexpr std::uint32_t s_sampledTextureBindingSlot  = 9u;
	static constexpr std::uint32_t s_samplerBindingSlot         = 10u;

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
