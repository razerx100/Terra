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
#include <DepthBuffer.hpp>
#include <VKRenderPass.hpp>
#include <ViewportAndScissorManager.hpp>
#include <Model.hpp>
#include <VkFramebuffer.hpp>
#include <Shader.hpp>
#include <TemporaryDataBuffer.hpp>
#include <Texture.hpp>

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

	[[nodiscard]]
	size_t AddMaterial(std::shared_ptr<Material> material);
	[[nodiscard]]
	std::vector<size_t> AddMaterials(std::vector<std::shared_ptr<Material>>&& materials);

	void UpdateMaterial(size_t index) const noexcept { m_materialBuffers.Update(index); }
	void RemoveMaterial(size_t index) noexcept { m_materialBuffers.Remove(index); }

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept;

	[[nodiscard]]
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

	void WaitForQueues() { m_graphicsQueue.WaitForQueueToFinish(); }

	[[nodiscard]]
	const VKSemaphore& GetGraphicsWait(size_t index) const noexcept { return m_graphicsWait.at(index); }

	virtual void SetShaderPath(const std::wstring& shaderPath) noexcept = 0;
	virtual void AddFragmentShader(const ShaderName& fragmentShader) = 0;
	virtual void ChangeFragmentShader(
		std::uint32_t modelBundleID, const ShaderName& fragmentShader
	) = 0;

	void BeginRenderPass(
		const VKCommandBuffer& graphicsCmdBuffer, const VKFramebuffer& frameBuffer,
		VkExtent2D renderArea
	);
	virtual void Render(
		size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea,
		std::uint64_t frameNumber, const VKSemaphore& imageWaitSemaphore
	) = 0;
	virtual void Resize(
		std::uint32_t width, std::uint32_t height,
		bool hasSwapchainFormatChanged, VkFormat swapchainFormat
	) = 0;
	virtual void SetMeshIndex(std::uint32_t modelBundleID, std::uint32_t meshBundleID) = 0;

	[[nodiscard]]
	virtual std::uint32_t AddModel(std::shared_ptr<ModelVS>&& model, const ShaderName& fragmentShader);
	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const ShaderName& fragmentShader
	);
	[[nodiscard]]
	virtual std::uint32_t AddModel(std::shared_ptr<ModelMS>&& model, const ShaderName& fragmentShader);
	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const ShaderName& fragmentShader
	);

	virtual void RemoveModelBundle(std::uint32_t bundleID) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle);
	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle);

	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

	[[nodiscard]]
	const VKRenderPass& GetRenderPass() const noexcept { return m_renderPass; }
	[[nodiscard]]
	const DepthBuffer& GetDepthBuffer() const noexcept { return m_depthBuffer; }

protected:
	[[nodiscard]]
	virtual std::uint32_t GetCameraBindingSlot() const noexcept = 0;
	[[nodiscard]]
	virtual std::uint32_t GetMaterialBindingSlot() const noexcept = 0;
	[[nodiscard]]
	virtual std::uint32_t GetCombinedTextureBindingSlot() const noexcept = 0;
	[[nodiscard]]
	virtual std::uint32_t GetSampledTextureBindingSlot() const noexcept = 0;
	[[nodiscard]]
	virtual std::uint32_t GetSamplerBindingSlot() const noexcept = 0;

	void SetCommonGraphicsDescriptorBufferLayout(VkShaderStageFlagBits cameraShaderStage) noexcept;

	virtual void Update(VkDeviceSize frameIndex) const noexcept;

protected:
	// These descriptors are bound to the Fragment shader. So, they should be the same across
	// all of the pipeline types. That's why we are going to bind them to their own setLayout.
	static constexpr std::uint32_t s_graphicsPipelineSetLayoutCount = 2u;
	static constexpr std::uint32_t s_vertexShaderSetLayoutIndex     = 0u;
	static constexpr std::uint32_t s_fragmentShaderSetLayoutIndex   = 1u;

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
	std::vector<VkDescriptorBuffer> m_graphicsDescriptorBuffers;
	TextureStorage                  m_textureStorage;
	TextureManager                  m_textureManager;
	MaterialBuffers                 m_materialBuffers;
	CameraManager                   m_cameraManager;
	DepthBuffer                     m_depthBuffer;
	VKRenderPass                    m_renderPass;
	VkClearColorValue               m_backgroundColour;
	ViewportAndScissorManager       m_viewportAndScissors;
	TemporaryDataBufferGPU          m_temporaryDataBuffer;

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
		m_graphicsDescriptorBuffers{ std::move(other.m_graphicsDescriptorBuffers) },
		m_textureStorage{ std::move(other.m_textureStorage) },
		m_textureManager{ std::move(other.m_textureManager) },
		m_materialBuffers{ std::move(other.m_materialBuffers) },
		m_cameraManager{ std::move(other.m_cameraManager) },
		m_depthBuffer{ std::move(other.m_depthBuffer) },
		m_renderPass{ std::move(other.m_renderPass) },
		m_backgroundColour{ other.m_backgroundColour },
		m_viewportAndScissors{ other.m_viewportAndScissors },
		m_temporaryDataBuffer{ std::move(other.m_temporaryDataBuffer) }
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
		m_graphicsDescriptorBuffers = std::move(other.m_graphicsDescriptorBuffers);
		m_textureStorage            = std::move(other.m_textureStorage);
		m_textureManager            = std::move(other.m_textureManager);
		m_materialBuffers           = std::move(other.m_materialBuffers);
		m_cameraManager             = std::move(other.m_cameraManager);
		m_depthBuffer               = std::move(other.m_depthBuffer);
		m_renderPass                = std::move(other.m_renderPass);
		m_backgroundColour          = other.m_backgroundColour;
		m_viewportAndScissors       = other.m_viewportAndScissors;
		m_temporaryDataBuffer       = std::move(other.m_temporaryDataBuffer);

		return *this;
	}
};

template<typename ModelManager_t, typename Derived>
class RenderEngineCommon : public RenderEngine
{
public:
	RenderEngineCommon(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
	) : RenderEngine{ deviceManager, std::move(threadPool), frameCount },
		m_modelManager{
			Derived::GetModelManager(
				deviceManager, &m_memoryManager, &m_stagingManager,
				static_cast<std::uint32_t>(frameCount)
			)
		}
	{
		for(auto& descriptorBuffer : m_graphicsDescriptorBuffers)
			m_textureManager.SetDescriptorBufferLayout(
				descriptorBuffer, GetCombinedTextureBindingSlot(), GetSampledTextureBindingSlot(),
				GetSamplerBindingSlot(), s_fragmentShaderSetLayoutIndex
		);
	}

	void SetShaderPath(const std::wstring& shaderPath) noexcept override
	{
		m_modelManager.SetShaderPath(shaderPath);
	}
	void AddFragmentShader(const ShaderName& fragmentShader) override
	{
		m_modelManager.AddPSO(fragmentShader);
	}
	void ChangeFragmentShader(
		std::uint32_t modelBundleID, const ShaderName& fragmentShader
	) override {
		m_modelManager.ChangePSO(modelBundleID, fragmentShader);
	}

	void RemoveModelBundle(std::uint32_t bundleID) noexcept override
	{
		m_modelManager.RemoveModelBundle(bundleID);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept override
	{
		m_modelManager.RemoveMeshBundle(bundleIndex);
	}

	void SetMeshIndex(std::uint32_t modelBundleID, std::uint32_t meshBundleID) override
	{
		m_modelManager.SetMeshIndex(modelBundleID, meshBundleID);
	}

	void Resize(
		std::uint32_t width, std::uint32_t height,
		bool hasSwapchainFormatChanged, VkFormat swapchainFormat
	) override {
		m_depthBuffer.Create(width, height);

		if (hasSwapchainFormatChanged)
		{
			m_renderPass.Create(
				RenderPassBuilder{}
				.AddColourAttachment(swapchainFormat)
				.AddDepthAttachment(m_depthBuffer.GetFormat())
				.Build()
			);

			// The model manager uses the render pass to create PSOs. So, if the renderPass is
			// changed, I will have to recreate all the PSOs as well. But the swapchain format
			// doesn't usually change, so it is not a major issue. But I should do it at some
			// point.
			m_modelManager.SetRenderPass(&m_renderPass);
		}

		m_viewportAndScissors.Resize(width, height);
	}

	[[nodiscard]]
	std::uint32_t GetCameraBindingSlot() const noexcept override
	{
		return Derived::s_cameraBindingSlot;
	}
	[[nodiscard]]
	std::uint32_t GetMaterialBindingSlot() const noexcept override
	{
		return Derived::s_materialBindingSlot;
	}
	[[nodiscard]]
	std::uint32_t GetCombinedTextureBindingSlot() const noexcept override
	{
		return Derived::s_combinedTextureBindingSlot;
	}
	[[nodiscard]]
	std::uint32_t GetSampledTextureBindingSlot() const noexcept override
	{
		return Derived::s_sampledTextureBindingSlot;
	}
	[[nodiscard]]
	std::uint32_t GetSamplerBindingSlot() const noexcept override
	{
		return Derived::s_samplerBindingSlot;
	}

protected:
	void Update(VkDeviceSize frameIndex) const noexcept override
	{
		RenderEngine::Update(frameIndex);

		m_modelManager.UpdatePerFrame(frameIndex);
	}

protected:
	ModelManager_t m_modelManager;

public:
	RenderEngineCommon(const RenderEngineCommon&) = delete;
	RenderEngineCommon& operator=(const RenderEngineCommon&) = delete;

	RenderEngineCommon(RenderEngineCommon&& other) noexcept
		: RenderEngine{ std::move(other) },
		m_modelManager{ std::move(other.m_modelManager) }
	{}
	RenderEngineCommon& operator=(RenderEngineCommon&& other) noexcept
	{
		RenderEngine::operator=(std::move(other));
		m_modelManager = std::move(other.m_modelManager);

		return *this;
	}
};
#endif
