#ifndef RENDER_ENGINE_HPP_
#define RENDER_ENGINE_HPP_
#include <memory>
#include <VkDeviceManager.hpp>
#include <VkCommandQueue.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <TextureManager.hpp>
#include <VkSharedBuffers.hpp>
#include <CameraManager.hpp>
#include <ViewportAndScissorManager.hpp>
#include <Model.hpp>
#include <VkFramebuffer.hpp>
#include <Shader.hpp>
#include <TemporaryDataBuffer.hpp>
#include <Texture.hpp>
#include <VkModelBuffer.hpp>
#include <PipelineManager.hpp>
#include <VkExternalRenderPass.hpp>
#include <VkExternalResourceManager.hpp>

// This needs to be a separate class, since the actual Engine will need the device to be created
// first. And these extensions must be added before the device is created. Each implemention may
// have different requirements. But you can't override a static function. And a virtual function
// can only be called after the object has been created.
class RenderEngineDeviceExtension
{
public:
	virtual ~RenderEngineDeviceExtension() = default;

	virtual void SetDeviceExtensions(VkDeviceExtensionManager& extensionManager) noexcept;
};

class RenderEngine
{
	// Getting the values from the same values from the deviceManager for each member is kinda
	// dumb, so keeping this constructor but making it private.
	RenderEngine(
		VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
		VkQueueFamilyMananger const* queueFamilyManager, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	);

public:
	RenderEngine(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	);
	virtual ~RenderEngine() = default;

	virtual void FinaliseInitialisation() = 0;

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	size_t AddTextureAsCombined(STexture&& texture);

	void UnbindCombinedTexture(size_t index);
	void UnbindCombinedTexture(size_t textureIndex, size_t samplerIndex);
	[[nodiscard]]
	std::uint32_t BindCombinedTexture(size_t index);
	[[nodiscard]]
	std::uint32_t BindCombinedTexture(size_t textureIndex, size_t samplerIndex);

	void RemoveTexture(size_t index);

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera> camera) noexcept
	{
		return m_cameraManager.AddCamera(std::move(camera));
	}
	void SetCamera(std::uint32_t index) noexcept { m_cameraManager.SetCamera(index); }
	void RemoveCamera(std::uint32_t index) noexcept { m_cameraManager.RemoveCamera(index); }

	virtual void SetShaderPath(const std::wstring& shaderPath) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline) = 0;

	virtual void ChangeModelPipelineInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) = 0;
	virtual void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept = 0;

	[[nodiscard]]
	// Returned semaphore will be signalled when the rendering is finished.
	virtual VkSemaphore Render(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, const VKSemaphore& imageWaitSemaphore
	) = 0;
	virtual void Resize(
		std::uint32_t width, std::uint32_t height, bool hasSwapchainFormatChanged
	) = 0;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	virtual std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle) = 0;

	virtual void RemoveModelBundle(std::uint32_t bundleIndex) noexcept = 0;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) = 0;

	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

public:
	// External stuff
	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept
	{
		return &m_externalResourceManager;
	}

	void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails);

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	);
	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
	);

	[[nodiscard]]
	virtual std::uint32_t AddExternalRenderPass() = 0;
	[[nodiscard]]
	virtual ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept = 0;
	[[nodiscard]]
	virtual std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(
		size_t index
	) const noexcept = 0;

	virtual void SetSwapchainExternalRenderPass() = 0;

	[[nodiscard]]
	virtual ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept = 0;
	[[nodiscard]]
	virtual std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept = 0;

	virtual void RemoveExternalRenderPass(size_t index) noexcept = 0;
	virtual void RemoveSwapchainExternalRenderPass() noexcept = 0;

	[[nodiscard]]
	virtual size_t GetActiveRenderPassCount() const noexcept = 0;

protected:
	[[nodiscard]]
	virtual std::uint32_t GetCameraBindingSlot() const noexcept = 0;
	virtual void ResetGraphicsPipeline() = 0;

	[[nodiscard]]
	static std::vector<std::uint32_t> AddModelsToBuffer(
		const ModelBundle& modelBundle, ModelBuffers& modelBuffers
	) noexcept;

	void SetCommonGraphicsDescriptorBufferLayout(VkShaderStageFlags cameraShaderStage) noexcept;

protected:
	// These descriptors are bound to the Fragment shader. So, they should be the same across
	// all of the pipeline types. That's why we are going to bind them to their own setLayout.
	static constexpr std::uint32_t s_graphicsPipelineSetLayoutCount = 3u;
	static constexpr std::uint32_t s_vertexShaderSetLayoutIndex     = 0u;
	static constexpr std::uint32_t s_fragmentShaderSetLayoutIndex   = 1u;

	// Set 0
	static constexpr std::uint32_t s_modelBuffersGraphicsBindingSlot = 0u;
	static constexpr std::uint32_t s_cameraBindingSlot               = 1u;

	// Set 1
	static constexpr std::uint32_t s_modelBuffersFragmentBindingSlot = 0u;

	static constexpr std::uint32_t s_combinedTextureBindingSlot = 1u;
	static constexpr std::uint32_t s_sampledTextureBindingSlot  = 2u;
	static constexpr std::uint32_t s_samplerBindingSlot         = 3u;

protected:
	std::shared_ptr<ThreadPool>     m_threadPool;
	MemoryManager                   m_memoryManager;
	VkGraphicsQueue                 m_graphicsQueue;
	std::vector<VKSemaphore>        m_graphicsWait;
	VkCommandQueue                  m_transferQueue;
	std::vector<VKSemaphore>        m_transferWait;
	StagingBufferManager            m_stagingManager;
	VkExternalResourceManager       m_externalResourceManager;
	std::vector<VkDescriptorBuffer> m_graphicsDescriptorBuffers;
	PipelineLayout                  m_graphicsPipelineLayout;
	TextureStorage                  m_textureStorage;
	TextureManager                  m_textureManager;
	CameraManager                   m_cameraManager;
	VkClearColorValue               m_backgroundColour;
	ViewportAndScissorManager       m_viewportAndScissors;
	TemporaryDataBufferGPU          m_temporaryDataBuffer;
	bool                            m_copyNecessary;

public:
	RenderEngine(const RenderEngine&) = delete;
	RenderEngine& operator=(const RenderEngine&) = delete;

	RenderEngine(RenderEngine&& other) noexcept
		: m_threadPool{ std::move(other.m_threadPool) },
		m_memoryManager{ std::move(other.m_memoryManager) },
		m_graphicsQueue{ std::move(other.m_graphicsQueue) },
		m_graphicsWait{ std::move(other.m_graphicsWait) },
		m_transferQueue{ std::move(other.m_transferQueue) },
		m_transferWait{ std::move(other.m_transferWait) },
		m_stagingManager{ std::move(other.m_stagingManager) },
		m_externalResourceManager{ std::move(other.m_externalResourceManager) },
		m_graphicsDescriptorBuffers{ std::move(other.m_graphicsDescriptorBuffers) },
		m_graphicsPipelineLayout{ std::move(other.m_graphicsPipelineLayout) },
		m_textureStorage{ std::move(other.m_textureStorage) },
		m_textureManager{ std::move(other.m_textureManager) },
		m_cameraManager{ std::move(other.m_cameraManager) },
		m_backgroundColour{ other.m_backgroundColour },
		m_viewportAndScissors{ other.m_viewportAndScissors },
		m_temporaryDataBuffer{ std::move(other.m_temporaryDataBuffer) },
		m_copyNecessary{ other.m_copyNecessary }
	{}
	RenderEngine& operator=(RenderEngine&& other) noexcept
	{
		m_threadPool                = std::move(other.m_threadPool);
		m_memoryManager             = std::move(other.m_memoryManager);
		m_graphicsQueue             = std::move(other.m_graphicsQueue);
		m_graphicsWait              = std::move(other.m_graphicsWait);
		m_transferQueue             = std::move(other.m_transferQueue);
		m_transferWait              = std::move(other.m_transferWait);
		m_stagingManager            = std::move(other.m_stagingManager);
		m_externalResourceManager   = std::move(other.m_externalResourceManager);
		m_graphicsDescriptorBuffers = std::move(other.m_graphicsDescriptorBuffers);
		m_graphicsPipelineLayout    = std::move(other.m_graphicsPipelineLayout);
		m_textureStorage            = std::move(other.m_textureStorage);
		m_textureManager            = std::move(other.m_textureManager);
		m_cameraManager             = std::move(other.m_cameraManager);
		m_backgroundColour          = other.m_backgroundColour;
		m_viewportAndScissors       = other.m_viewportAndScissors;
		m_temporaryDataBuffer       = std::move(other.m_temporaryDataBuffer);
		m_copyNecessary             = other.m_copyNecessary;

		return *this;
	}
};

template<
	typename ModelManager_t,
	typename MeshManager_t,
	typename GraphicsPipeline_t,
	typename Derived
>
class RenderEngineCommon : public RenderEngine
{
protected:
	using ExternalRenderPass_t   = VkExternalRenderPassCommon<ModelManager_t>;
	using ExternalRenderPassSP_t = std::shared_ptr<ExternalRenderPass_t>;

public:
	RenderEngineCommon(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount,
		ModelManager_t&& modelManager, ModelBuffers&& modelBuffers
	) : RenderEngine{ deviceManager, std::move(threadPool), frameCount },
		m_modelManager{ std::move(modelManager) },
		m_modelBuffers{ std::move(modelBuffers) },
		m_meshManager{
			deviceManager.GetLogicalDevice(), &m_memoryManager,
			deviceManager.GetQueueFamilyManager().GetAllIndices()
		},
		m_graphicsPipelineManager{ deviceManager.GetLogicalDevice() },
		m_renderPasses{}, m_swapchainRenderPass{}
	{
		for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
			m_textureManager.SetDescriptorBufferLayout(
				descriptorBuffer, s_combinedTextureBindingSlot, s_sampledTextureBindingSlot,
				s_samplerBindingSlot, s_fragmentShaderSetLayoutIndex
			);
	}

	void SetShaderPath(const std::wstring& shaderPath) override
	{
		m_graphicsPipelineManager.SetShaderPath(shaderPath);
	}

	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline) override
	{
		return m_graphicsPipelineManager.AddOrGetGraphicsPipeline(gfxPipeline);
	}

	void ChangeModelPipelineInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t modelIndex,
		std::uint32_t oldPipelineIndex, std::uint32_t newPipelineIndex
	) override {
		m_modelManager.ChangeModelPipeline(
			modelBundleIndex, modelIndex, oldPipelineIndex, newPipelineIndex
		);
	}

	void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept override
	{
		m_graphicsPipelineManager.SetOverwritable(pipelineIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_meshManager.RemoveMeshBundle(bundleIndex);
	}

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_modelBuffers.Remove(m_modelManager.RemoveModelBundle(bundleIndex));
	}

	void Resize(std::uint32_t width, std::uint32_t height, bool hasSwapchainFormatChanged) override
	{
		if (hasSwapchainFormatChanged)
			m_graphicsPipelineManager.RecreateAllGraphicsPipelines();

		m_viewportAndScissors.Resize(width, height);
	}

	[[nodiscard]]
	std::uint32_t GetCameraBindingSlot() const noexcept override
	{
		return Derived::s_cameraBindingSlot;
	}

	[[nodiscard]]
	VkSemaphore Render(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, const VKSemaphore& imageWaitSemaphore
	) final {
		// Wait for the previous Graphics command buffer to finish.
		m_graphicsQueue.WaitForSubmission(frameIndex);
		// It should be okay to clear the data now that the frame has finished
		// its submission.
		m_temporaryDataBuffer.Clear(frameIndex);

		// This should be fine. But putting this as a reminder, that
		// the presentation engine might still be running and using some resources.
		Update(static_cast<VkDeviceSize>(frameIndex));

		VkSemaphore waitSemaphore = imageWaitSemaphore.Get();

		waitSemaphore = static_cast<Derived*>(this)->ExecutePipelineStages(
			frameIndex, renderTarget, renderArea, semaphoreCounter, waitSemaphore
		);

		return waitSemaphore;
	}

	[[nodiscard]]
	std::uint32_t AddExternalRenderPass() override
	{
		return static_cast<std::uint32_t>(
			m_renderPasses.Add(
				std::make_shared<ExternalRenderPass_t>(
					&m_modelManager, m_externalResourceManager.GetVkResourceFactory()
				)
			)
		);
	}
	[[nodiscard]]
	ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept override
	{
		return m_renderPasses[index].get();
	}
	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(size_t index) const noexcept override
	{
		return m_renderPasses[index];
	}

	void RemoveExternalRenderPass(size_t index) noexcept override
	{
		m_renderPasses[index].reset();
		m_renderPasses.RemoveElement(index);
	}

	void SetSwapchainExternalRenderPass() override
	{
		m_swapchainRenderPass = std::make_shared<ExternalRenderPass_t>(
			&m_modelManager, m_externalResourceManager.GetVkResourceFactory()
		);
	}

	[[nodiscard]]
	ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept override
	{
		return m_swapchainRenderPass.get();
	}

	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept override
	{
		return m_swapchainRenderPass;
	}

	void RemoveSwapchainExternalRenderPass() noexcept override
	{
		m_swapchainRenderPass.reset();
	}

	[[nodiscard]]
	size_t GetActiveRenderPassCount() const noexcept override
	{
		size_t activeRenderPassCount = m_renderPasses.GetIndicesManager().GetActiveIndexCount();

		if (m_swapchainRenderPass)
			++activeRenderPassCount;

		return activeRenderPassCount;
	}

protected:
	void Update(VkDeviceSize frameIndex) noexcept
	{
		m_cameraManager.Update(frameIndex);

		static_cast<Derived*>(this)->_updatePerFrame(frameIndex);

		m_externalResourceManager.UpdateExtensionData(static_cast<size_t>(frameIndex));
	}

	void CreateGraphicsPipelineLayout()
	{
		if (!std::empty(m_graphicsDescriptorBuffers))
			m_graphicsPipelineLayout.Create(m_graphicsDescriptorBuffers.front().GetValidLayouts());

		m_graphicsPipelineManager.SetPipelineLayout(m_graphicsPipelineLayout.Get());
	}

	void ResetGraphicsPipeline() override
	{
		CreateGraphicsPipelineLayout();

		m_graphicsPipelineManager.RecreateAllGraphicsPipelines();
	}

protected:
	ModelManager_t                         m_modelManager;
	ModelBuffers                           m_modelBuffers;
	MeshManager_t                          m_meshManager;
	PipelineManager<GraphicsPipeline_t>    m_graphicsPipelineManager;
	ReusableVector<ExternalRenderPassSP_t> m_renderPasses;
	ExternalRenderPassSP_t                 m_swapchainRenderPass;

public:
	RenderEngineCommon(const RenderEngineCommon&) = delete;
	RenderEngineCommon& operator=(const RenderEngineCommon&) = delete;

	RenderEngineCommon(RenderEngineCommon&& other) noexcept
		: RenderEngine{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) },
		m_modelBuffers{ std::move(other.m_modelBuffers) },
		m_meshManager{ std::move(other.m_meshManager) },
		m_graphicsPipelineManager{ std::move(other.m_graphicsPipelineManager) },
		m_renderPasses{ std::move(other.m_renderPasses) },
		m_swapchainRenderPass{ std::move(other.m_swapchainRenderPass) }
	{}
	RenderEngineCommon& operator=(RenderEngineCommon&& other) noexcept
	{
		RenderEngine::operator=(std::move(other));
		m_modelManager            = std::move(other.m_modelManager);
		m_modelBuffers            = std::move(other.m_modelBuffers);
		m_meshManager             = std::move(other.m_meshManager);
		m_graphicsPipelineManager = std::move(other.m_graphicsPipelineManager);
		m_renderPasses            = std::move(other.m_renderPasses);
		m_swapchainRenderPass     = std::move(other.m_swapchainRenderPass);

		return *this;
	}
};
#endif
