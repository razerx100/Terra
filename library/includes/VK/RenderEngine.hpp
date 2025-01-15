#ifndef RENDER_ENGINE_HPP_
#define RENDER_ENGINE_HPP_
#include <memory>
#include <VkDeviceManager.hpp>
#include <VkCommandQueue.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <TextureManager.hpp>
#include <CommonBuffers.hpp>
#include <CameraManager.hpp>
#include <ViewportAndScissorManager.hpp>
#include <Model.hpp>
#include <VkFramebuffer.hpp>
#include <Shader.hpp>
#include <TemporaryDataBuffer.hpp>
#include <Texture.hpp>
#include <VkModelBuffer.hpp>
#include <VkRenderPassManager.hpp>
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

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	size_t AddMaterial(std::shared_ptr<Material> material);
	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	std::vector<size_t> AddMaterials(std::vector<std::shared_ptr<Material>>&& materials);

	void UpdateMaterial(size_t index) const noexcept { m_materialBuffers.Update(index); }
	void RemoveMaterial(size_t index) noexcept { m_materialBuffers.Remove(index); }

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
	virtual void AddFragmentShader(const ShaderName& fragmentShader) = 0;
	virtual void ChangeFragmentShader(
		std::uint32_t modelBundleID, const ShaderName& fragmentShader
	) = 0;

	[[nodiscard]]
	// Returned semaphore will be signalled when the rendering is finished.
	virtual VkSemaphore Render(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, const VKSemaphore& imageWaitSemaphore
	) = 0;
	virtual void Resize(
		std::uint32_t width, std::uint32_t height,
		bool hasSwapchainFormatChanged, VkFormat swapchainFormat
	) = 0;

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	virtual std::uint32_t AddModelBundle(
		std::shared_ptr<ModelBundle>&& modelBundle, const ShaderName& fragmentShader
	) = 0;

	virtual void RemoveModelBundle(std::uint32_t bundleID) noexcept = 0;

	[[nodiscard]]
	ExternalResourceFactory const* GetExternalResourceFactory() const noexcept
	{
		return m_externalResourceManager.GetResourceFactory();
	}

	[[nodiscard]]
	// Should wait for the device to be idle before calling this.
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle) = 0;

	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

protected:
	[[nodiscard]]
	virtual std::uint32_t GetCameraBindingSlot() const noexcept = 0;
	virtual void ResetGraphicsPipeline() = 0;

	void SetCommonGraphicsDescriptorBufferLayout(VkShaderStageFlags cameraShaderStage) noexcept;

	virtual void Update(VkDeviceSize frameIndex) const noexcept;

protected:
	// These descriptors are bound to the Fragment shader. So, they should be the same across
	// all of the pipeline types. That's why we are going to bind them to their own setLayout.
	static constexpr std::uint32_t s_graphicsPipelineSetLayoutCount = 2u;
	static constexpr std::uint32_t s_vertexShaderSetLayoutIndex     = 0u;
	static constexpr std::uint32_t s_fragmentShaderSetLayoutIndex   = 1u;

	// The fragment and Vertex data are on different sets. So both can be 0u.
	static constexpr std::uint32_t s_modelBuffersFragmentBindingSlot = 0u;
	static constexpr std::uint32_t s_modelBuffersGraphicsBindingSlot = 0u;

	static constexpr std::uint32_t s_materialBindingSlot        = 1u;
	static constexpr std::uint32_t s_combinedTextureBindingSlot = 2u;
	static constexpr std::uint32_t s_sampledTextureBindingSlot  = 3u;
	static constexpr std::uint32_t s_samplerBindingSlot         = 4u;

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
	MaterialBuffers                 m_materialBuffers;
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
		m_materialBuffers{ std::move(other.m_materialBuffers) },
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
		m_materialBuffers           = std::move(other.m_materialBuffers);
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
public:
	RenderEngineCommon(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	) : RenderEngine{ deviceManager, std::move(threadPool), frameCount },
		m_modelManager{
			Derived::GetModelManager(
				deviceManager, &m_memoryManager, static_cast<std::uint32_t>(frameCount)
			)
		},
		m_meshManager{
			deviceManager.GetLogicalDevice(), &m_memoryManager,
			deviceManager.GetQueueFamilyManager().GetAllIndices()
		},
		m_renderPassManager{ deviceManager.GetLogicalDevice() },
		m_modelBuffers{
			Derived::ConstructModelBuffers(
				deviceManager, &m_memoryManager, static_cast<std::uint32_t>(frameCount)
			)
		}
	{
		m_renderPassManager.SetDepthTesting(deviceManager.GetLogicalDevice(), &m_memoryManager);

		for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
			m_textureManager.SetDescriptorBufferLayout(
				descriptorBuffer, s_combinedTextureBindingSlot, s_sampledTextureBindingSlot,
				s_samplerBindingSlot, s_fragmentShaderSetLayoutIndex
			);
	}

	void SetShaderPath(const std::wstring& shaderPath) override
	{
		m_renderPassManager.SetShaderPath(shaderPath);
	}
	void AddFragmentShader(const ShaderName& fragmentShader) override
	{
		m_renderPassManager.AddOrGetGraphicsPipeline(fragmentShader);
	}
	void ChangeFragmentShader(
		std::uint32_t modelBundleID, const ShaderName& fragmentShader
	) override {
		const std::uint32_t psoIndex = m_renderPassManager.AddOrGetGraphicsPipeline(fragmentShader);

		m_modelManager.ChangePSO(modelBundleID, psoIndex);
	}

	void RemoveModelBundle(std::uint32_t bundleID) noexcept override
	{
		m_modelManager.RemoveModelBundle(bundleID, m_modelBuffers);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_meshManager.RemoveMeshBundle(bundleIndex);
	}

	void Resize(
		std::uint32_t width, std::uint32_t height,
		bool hasSwapchainFormatChanged, VkFormat swapchainFormat
	) override {
		m_renderPassManager.ResizeDepthBuffer(width, height);

		if (hasSwapchainFormatChanged)
		{
			m_renderPassManager.SetColourAttachmentFormat(swapchainFormat);
			m_renderPassManager.RecreatePipelines();
		}

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

protected:
	void Update(VkDeviceSize frameIndex) const noexcept override
	{
		RenderEngine::Update(frameIndex);

		m_modelBuffers.Update(frameIndex);
		static_cast<Derived const*>(this)->_updatePerFrame(frameIndex);
	}

	void CreateGraphicsPipelineLayout()
	{
		if (!std::empty(m_graphicsDescriptorBuffers))
			m_graphicsPipelineLayout.Create(m_graphicsDescriptorBuffers.front().GetLayouts());

		m_renderPassManager.SetPipelineLayout(m_graphicsPipelineLayout.Get());
	}

	void ResetGraphicsPipeline() override
	{
		CreateGraphicsPipelineLayout();

		m_renderPassManager.RecreatePipelines();
	}

protected:
	ModelManager_t                          m_modelManager;
	MeshManager_t                           m_meshManager;
	VkRenderPassManager<GraphicsPipeline_t> m_renderPassManager;
	ModelBuffers                            m_modelBuffers;

public:
	RenderEngineCommon(const RenderEngineCommon&) = delete;
	RenderEngineCommon& operator=(const RenderEngineCommon&) = delete;

	RenderEngineCommon(RenderEngineCommon&& other) noexcept
		: RenderEngine{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) },
		m_meshManager{ std::move(other.m_meshManager) },
		m_renderPassManager{ std::move(other.m_renderPassManager) },
		m_modelBuffers{ std::move(other.m_modelBuffers) }
	{}
	RenderEngineCommon& operator=(RenderEngineCommon&& other) noexcept
	{
		RenderEngine::operator=(std::move(other));
		m_modelManager      = std::move(other.m_modelManager);
		m_meshManager       = std::move(other.m_meshManager);
		m_renderPassManager = std::move(other.m_renderPassManager);
		m_modelBuffers      = std::move(other.m_modelBuffers);

		return *this;
	}
};
#endif
