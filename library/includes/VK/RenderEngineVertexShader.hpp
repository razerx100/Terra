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
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& fragmentShader
	) override;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;

	void Render(
		size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
		std::uint64_t frameNumber, const VKSemaphore& imageWaitSemaphore
	) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndividual GetModelManager(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

private:
	static constexpr std::uint32_t s_cameraBindingSlot = 1u;

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
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundleVS>&& modelBundle, const ShaderName& fragmentShader
	) override;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle) override;

	void Render(
		size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
		std::uint64_t frameNumber, const VKSemaphore& imageWaitSemaphore
	) override;

private:
	[[nodiscard]]
	static ModelManagerVSIndirect GetModelManager(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
		StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
	);

private:
	// Graphics
	static constexpr std::uint32_t s_cameraBindingSlot        = 2u;

	// Compute
	static constexpr std::uint32_t s_computePipelineSetLayoutCount = 1u;
	static constexpr std::uint32_t s_computeShaderSetLayoutIndex   = 0u;

	static constexpr std::uint32_t s_cameraComputeBindingSlot = 10u;

private:
	VkCommandQueue                  m_computeQueue;
	std::vector<VKSemaphore>        m_computeWait;
	std::vector<VkDescriptorBuffer> m_computeDescriptorBuffers;

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
