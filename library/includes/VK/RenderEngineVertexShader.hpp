#ifndef RENDER_ENGINE_VERTEX_SHADER_HPP_
#define RENDER_ENGINE_VERTEX_SHADER_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineVSIndividualDeviceExtension : public RenderEngineDeviceExtension {};

class RenderEngineVSIndividual : public RenderEngine
{
public:
	RenderEngineVSIndividual(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	void SetShaderPath(const std::wstring& shaderPath) noexcept override
	{
		m_modelManager.SetShaderPath(shaderPath);
	}
	void AddFragmentShader(const std::wstring& fragmentShader) override
	{
		m_modelManager.AddPSO(fragmentShader);
	}
	void ChangeFragmentShader(
		std::uint32_t modelBundleID, const std::wstring& fragmentShader
	) override {
		m_modelManager.ChangePSO(modelBundleID, fragmentShader);
	}

	[[nodiscard]]
	std::uint32_t AddModel(
		std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader
	) override;
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
	) override;

	void RemoveModelBundle(std::uint32_t bundleID) noexcept override
	{
		m_modelManager.RemoveModelBundle(bundleID);
	}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override
	{
		return m_modelManager.AddMeshBundle(std::move(meshBundle), m_stagingManager);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_modelManager.RemoveMeshBundle(bundleIndex);
	}

	void Render(size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea) override;

private:
	// These might not work. As a binding slot with variable descriptor count must be at the end.
	// But lets see.
	static constexpr std::uint32_t s_combinedTextureBindingSlot = 1u;
	static constexpr std::uint32_t s_sampledTextureBindingSlot  = 2u;
	static constexpr std::uint32_t s_samplerBindingSlot         = 3u;

private:
	void Update(VkDeviceSize frameIndex) const noexcept override;

	[[nodiscard]]
	std::uint32_t GetCombinedTextureBindingSlot() const noexcept override
	{ return s_combinedTextureBindingSlot; }
	[[nodiscard]]
	std::uint32_t GetSampledTextureBindingSlot() const noexcept override
	{ return s_sampledTextureBindingSlot; }
	[[nodiscard]]
	std::uint32_t GetSamplerBindingSlot() const noexcept override { return s_samplerBindingSlot; }

private:
	ModelManagerVSIndividual m_modelManager;

public:
	RenderEngineVSIndividual(const RenderEngineVSIndividual&) = delete;
	RenderEngineVSIndividual& operator=(const RenderEngineVSIndividual&) = delete;

	RenderEngineVSIndividual(RenderEngineVSIndividual&& other) noexcept
		: RenderEngine{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) }
	{}
	RenderEngineVSIndividual& operator=(RenderEngineVSIndividual&& other) noexcept
	{
		RenderEngine::operator=(std::move(other));
		m_modelManager = std::move(other.m_modelManager);

		return *this;
	}
};
#endif
