#ifndef RENDER_ENGINE_VERTEX_SHADER_HPP_
#define RENDER_ENGINE_VERTEX_SHADER_HPP_
#include <RenderEngine.hpp>
#include <ModelManager.hpp>

class RenderEngineVSIndividualDeviceExtension : public RenderEngineDeviceExtension {};

class RenderEngineVSIndividual
	: public RenderEngineCommon<ModelManagerVSIndividual, RenderEngineVSIndividual>
{
	friend class RenderEngineCommon<ModelManagerVSIndividual, RenderEngineVSIndividual>;

public:
	RenderEngineVSIndividual(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	[[nodiscard]]
	std::uint32_t AddModel(
		std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader
	) override;
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
	) override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;

	void Render(size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndividual GetModelManager(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

private:
	// These might not work. As a binding slot with variable descriptor count must be at the end.
	// But lets see.
	static constexpr std::uint32_t s_combinedTextureBindingSlot = 1u;
	static constexpr std::uint32_t s_sampledTextureBindingSlot  = 2u;
	static constexpr std::uint32_t s_samplerBindingSlot         = 3u;

private:
	[[nodiscard]]
	std::uint32_t GetCombinedTextureBindingSlot() const noexcept override
	{ return s_combinedTextureBindingSlot; }
	[[nodiscard]]
	std::uint32_t GetSampledTextureBindingSlot() const noexcept override
	{ return s_sampledTextureBindingSlot; }
	[[nodiscard]]
	std::uint32_t GetSamplerBindingSlot() const noexcept override { return s_samplerBindingSlot; }

public:
	RenderEngineVSIndividual(const RenderEngineVSIndividual&) = delete;
	RenderEngineVSIndividual& operator=(const RenderEngineVSIndividual&) = delete;

	RenderEngineVSIndividual(RenderEngineVSIndividual&& other) noexcept
		: RenderEngineCommon{ std::move(other) }
	{}
	RenderEngineVSIndividual& operator=(RenderEngineVSIndividual&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));

		return *this;
	}
};

class RenderEngineVSIndirectDeviceExtension : public RenderEngineDeviceExtension {};

class RenderEngineVSIndirect
	: public RenderEngineCommon<ModelManagerVSIndirect, RenderEngineVSIndirect>
{
	friend class RenderEngineCommon<ModelManagerVSIndirect, RenderEngineVSIndirect>;

public:
	RenderEngineVSIndirect(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);

	[[nodiscard]]
	std::uint32_t AddModel(
		std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader
	) override;
	[[nodiscard]]
	std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
	) override;

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;

	void Render(size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndirect GetModelManager(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

private:
	// These might not work. As a binding slot with variable descriptor count must be at the end.
	// But lets see.
	static constexpr std::uint32_t s_combinedTextureBindingSlot = 2u;
	static constexpr std::uint32_t s_sampledTextureBindingSlot  = 3u;
	static constexpr std::uint32_t s_samplerBindingSlot         = 4u;

private:
	VkCommandQueue                  m_computeQueue;
	std::vector<VKSemaphore>        m_computeWait;
	std::vector<VkDescriptorBuffer> m_computeDescriptorBuffers;

private:
	[[nodiscard]]
	std::uint32_t GetCombinedTextureBindingSlot() const noexcept override
	{ return s_combinedTextureBindingSlot; }
	[[nodiscard]]
	std::uint32_t GetSampledTextureBindingSlot() const noexcept override
	{ return s_sampledTextureBindingSlot; }
	[[nodiscard]]
	std::uint32_t GetSamplerBindingSlot() const noexcept override { return s_samplerBindingSlot; }

public:
	RenderEngineVSIndirect(const RenderEngineVSIndirect&) = delete;
	RenderEngineVSIndirect& operator=(const RenderEngineVSIndirect&) = delete;

	RenderEngineVSIndirect(RenderEngineVSIndirect&& other) noexcept
		: RenderEngineCommon{ std::move(other) },
		m_computeQueue{ std::move(other.m_computeQueue) },
		m_computeWait{ std::move(other.m_computeWait) },
		m_computeDescriptorBuffers{ std::move(other.m_computeDescriptorBuffers) }
	{}
	RenderEngineVSIndirect& operator=(RenderEngineVSIndirect&& other) noexcept
	{
		RenderEngineCommon::operator=(std::move(other));
		m_computeQueue             = std::move(other.m_computeQueue);
		m_computeWait              = std::move(other.m_computeWait);
		m_computeDescriptorBuffers = std::move(other.m_computeDescriptorBuffers);

		return *this;
	}
};
#endif
