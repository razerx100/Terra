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

namespace RenderEngineDeviceExtension
{
	void SetDeviceExtensions(VkDeviceExtensionManager& extensionManager) noexcept;
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
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	);

	// Should wait for the device to be idle before calling this.
	[[nodiscard]]
	size_t AddTextureAsCombined(STexture&& texture);

	void UnbindCombinedTexture(size_t textureIndex, std::uint32_t bindingIndex);

	void UnbindCombinedTexture(
		size_t textureIndex, std::uint32_t bindingIndex, size_t samplerIndex
	);

	template<class Derived>
	[[nodiscard]]
	std::uint32_t BindCombinedTexture(this Derived& self, size_t textureIndex)
	{
		return self.BindCombinedTexture(
			textureIndex, self.m_textureStorage.GetDefaultSamplerIndex()
		);
	}

	template<class Derived>
	[[nodiscard]]
	std::uint32_t BindCombinedTexture(
		this Derived& self, size_t textureIndex, size_t samplerIndex
	) {
		VkTextureView const* textureView = self.m_textureStorage.GetPtr(textureIndex);
		VKSampler const* sampler         = self.m_textureStorage.GetSamplerPtr(samplerIndex);

		// The current caching system only works for read only single textures which are bound to
		// multiple descriptor buffers. Because we only cache one of them.
		std::optional<std::uint32_t> localCacheIndex
			= self.m_textureStorage.GetAndRemoveCombinedLocalDescIndex(
				static_cast<std::uint32_t>(textureIndex),
				static_cast<std::uint32_t>(samplerIndex)
			);

		return self.BindCombinedTextureCommon(textureView, sampler, localCacheIndex);
	}

	void UnbindExternalTexture(std::uint32_t bindingIndex);

	void RebindExternalTexture(size_t textureIndex, std::uint32_t bindingIndex);
	void RebindExternalTexture(
		size_t textureIndex, size_t samplerIndex, std::uint32_t bindingIndex
	);

	template<class Derived>
	[[nodiscard]]
	std::uint32_t BindExternalTexture(this Derived& self, size_t textureIndex)
	{
		return self.BindExternalTexture(
			textureIndex, self.m_textureStorage.GetDefaultSamplerIndex()
		);
	}

	template<class Derived>
	[[nodiscard]]
	std::uint32_t BindExternalTexture(
		this Derived& self, size_t textureIndex, size_t samplerIndex
	) {
		VkExternalResourceFactory* resourceFactory
			= self.m_externalResourceManager->GetVkResourceFactory();

		VkTextureView const* textureView = &resourceFactory->GetVkTextureView(textureIndex);

		VKSampler const* sampler = self.m_textureStorage.GetSamplerPtr(samplerIndex);

		// Can't cache as the underlying resource might change or we might have a separate texture
		// on each descriptor buffer.
		return self.BindCombinedTextureCommon(textureView, sampler, {});
	}

	void RemoveTexture(size_t textureIndex);

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera> camera) noexcept
	{
		return m_cameraManager.AddCamera(std::move(camera));
	}
	void SetCamera(std::uint32_t index) noexcept { m_cameraManager.SetCamera(index); }
	void RemoveCamera(std::uint32_t index) noexcept { m_cameraManager.RemoveCamera(index); }

private:
	template<class Derived>
	[[nodiscard]]
	std::uint32_t BindCombinedTextureCommon(
		this Derived& self, VkTextureView const* textureView, VKSampler const* sampler,
		std::optional<std::uint32_t> oLocalCacheIndex
	) {
		static constexpr VkDescriptorType DescType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		static constexpr TextureDescType TexDescType = TextureDescType::CombinedTexture;

		std::optional<size_t> oFreeGlobalDescIndex
			= self.m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

		// If there is no free global index, increase the limit. Right now it should be possible to
		// have 65535 bound textures at once. There could be more textures.
		if (!oFreeGlobalDescIndex)
		{
			self.m_textureManager.IncreaseMaximumBindingCount<TexDescType>();

			for (VkDescriptorBuffer& descriptorBuffer : self.m_graphicsDescriptorBuffers)
			{
				const std::vector<VkDescriptorSetLayoutBinding> oldSetLayoutBindings
					= descriptorBuffer.GetLayout(s_fragmentShaderSetLayoutIndex).GetBindings();

				self.m_textureManager.SetDescriptorBufferLayout(
					descriptorBuffer, s_combinedTextureBindingSlot, s_sampledTextureBindingSlot,
					s_samplerBindingSlot, s_fragmentShaderSetLayoutIndex
				);

				descriptorBuffer.RecreateSetLayout(
					s_fragmentShaderSetLayoutIndex, oldSetLayoutBindings
				);
			}

			oFreeGlobalDescIndex = self.m_textureManager.GetFreeGlobalDescriptorIndex<DescType>();

			self.ResetGraphicsPipeline();
		}

		const auto freeGlobalDescIndex = static_cast<std::uint32_t>(oFreeGlobalDescIndex.value());

		self.m_textureManager.SetBindingAvailability<DescType>(freeGlobalDescIndex, false);

		if (oLocalCacheIndex)
		{
			const std::uint32_t localCacheIndex = oLocalCacheIndex.value();

			void const* localDescriptor = self.m_textureManager.GetLocalDescriptor<DescType>(
				localCacheIndex
			);

			self.m_textureManager.SetLocalDescriptorAvailability<DescType>(localCacheIndex, true);

			for (VkDescriptorBuffer& descriptorBuffer : self.m_graphicsDescriptorBuffers)
				descriptorBuffer.SetCombinedImageDescriptor(
					localDescriptor, s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex,
					freeGlobalDescIndex
				);
		}
		else
			for (VkDescriptorBuffer& descriptorBuffer : self.m_graphicsDescriptorBuffers)
				descriptorBuffer.SetCombinedImageDescriptor(
					*textureView, *sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					s_combinedTextureBindingSlot, s_fragmentShaderSetLayoutIndex,
					freeGlobalDescIndex
				);

		return freeGlobalDescIndex;
	}

public:
	// External stuff
	[[nodiscard]]
	ExternalResourceManager* GetExternalResourceManager() noexcept
	{
		return m_externalResourceManager.get();
	}

	void UpdateExternalBufferDescriptor(const ExternalBufferBindingDetails& bindingDetails);

	void UploadExternalBufferGPUOnlyData(
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData,
		size_t srcDataSizeInBytes, size_t dstBufferOffset
	);
	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes
	);

protected:
	[[nodiscard]]
	static std::vector<std::uint32_t> AddModelsToBuffer(
		const ModelBundle& modelBundle, ModelBuffers& modelBuffers
	) noexcept;

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
	std::shared_ptr<ThreadPool>                m_threadPool;
	// The pointer to this is shared in different places. So, if I make it a automatic
	// member, the kept pointers would be invalid after a move.
	std::unique_ptr<MemoryManager>             m_memoryManager;
	VkGraphicsQueue                            m_graphicsQueue;
	std::vector<VKSemaphore>                   m_graphicsWait;
	VkCommandQueue                             m_transferQueue;
	std::vector<VKSemaphore>                   m_transferWait;
	StagingBufferManager                       m_stagingManager;
	std::unique_ptr<VkExternalResourceManager> m_externalResourceManager;
	std::vector<VkDescriptorBuffer>            m_graphicsDescriptorBuffers;
	PipelineLayout                             m_graphicsPipelineLayout;
	TextureStorage                             m_textureStorage;
	TextureManager                             m_textureManager;
	CameraManager                              m_cameraManager;
	ViewportAndScissorManager                  m_viewportAndScissors;
	TemporaryDataBufferGPU                     m_temporaryDataBuffer;
	bool                                       m_copyNecessary;

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
	friend class RenderEngine;

protected:
	using ExternalRenderPass_t   = VkExternalRenderPassCommon<ModelManager_t>;
	using ExternalRenderPassSP_t = std::shared_ptr<ExternalRenderPass_t>;

public:
	RenderEngineCommon(
		const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
	) : RenderEngine{ deviceManager, std::move(threadPool), frameCount },
		m_modelManager{},
		m_modelBuffers{
			CreateModelBuffers(deviceManager, m_memoryManager.get(), frameCount)
		},
		m_meshManager{
			deviceManager.GetLogicalDevice(), m_memoryManager.get(),
			deviceManager.GetQueueFamilyManager().GetAllIndices()
		},
		m_graphicsPipelineManager{ deviceManager.GetLogicalDevice() },
		m_renderPasses{}, m_swapchainRenderPass{}
	{
		for (VkDescriptorBuffer& descriptorBuffer : m_graphicsDescriptorBuffers)
			m_textureManager.SetDescriptorBufferLayout(
				descriptorBuffer, s_combinedTextureBindingSlot, s_sampledTextureBindingSlot,
				s_samplerBindingSlot, s_fragmentShaderSetLayoutIndex
			);
	}

	[[nodiscard]]
	std::uint32_t AddGraphicsPipeline(const ExternalGraphicsPipeline& gfxPipeline)
	{
		return m_graphicsPipelineManager.AddOrGetGraphicsPipeline(gfxPipeline);
	}

	void ReconfigureModelPipelinesInBundle(
		std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
		std::uint32_t increasedModelsPipelineIndex
	) {
		m_modelManager->ReconfigureModels(
			modelBundleIndex, decreasedModelsPipelineIndex, increasedModelsPipelineIndex
		);
	}

	void RemoveGraphicsPipeline(std::uint32_t pipelineIndex) noexcept
	{
		m_graphicsPipelineManager.SetOverwritable(pipelineIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
	{
		m_meshManager.RemoveMeshBundle(bundleIndex);
	}

	void RemoveModelBundle(std::uint32_t bundleIndex) noexcept
	{
		std::shared_ptr<ModelBundle> modelBundle = m_modelManager->RemoveModelBundle(bundleIndex);

		const auto& models = modelBundle->GetModels();

		for (const std::shared_ptr<Model>& model : models)
			m_modelBuffers.Remove(model->GetModelIndexInBuffer());
	}

	void Resize(std::uint32_t width, std::uint32_t height, bool hasSwapchainFormatChanged)
	{
		if (hasSwapchainFormatChanged)
			m_graphicsPipelineManager.RecreateAllGraphicsPipelines();

		m_viewportAndScissors.Resize(width, height);
	}

	[[nodiscard]]
	VkSemaphore Render(
		size_t frameIndex, const VKImageView& renderTarget, VkExtent2D renderArea,
		std::uint64_t& semaphoreCounter, const VKSemaphore& imageWaitSemaphore
	) {
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
	std::uint32_t AddExternalRenderPass()
	{
		return static_cast<std::uint32_t>(
			m_renderPasses.Add(
				std::make_shared<ExternalRenderPass_t>(
					m_modelManager.get(), m_externalResourceManager->GetVkResourceFactory()
				)
			)
		);
	}

	[[nodiscard]]
	ExternalRenderPass* GetExternalRenderPassRP(size_t index) const noexcept
	{
		return m_renderPasses[index].get();
	}

	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetExternalRenderPassSP(size_t index) const noexcept
	{
		return m_renderPasses[index];
	}

	void RemoveExternalRenderPass(size_t index) noexcept
	{
		m_renderPasses[index].reset();
		m_renderPasses.RemoveElement(index);
	}

	void SetSwapchainExternalRenderPass()
	{
		m_swapchainRenderPass = std::make_shared<ExternalRenderPass_t>(
			m_modelManager.get(), m_externalResourceManager->GetVkResourceFactory()
		);
	}

	[[nodiscard]]
	ExternalRenderPass* GetSwapchainExternalRenderPassRP() const noexcept
	{
		return m_swapchainRenderPass.get();
	}

	[[nodiscard]]
	std::shared_ptr<ExternalRenderPass> GetSwapchainExternalRenderPassSP() const noexcept
	{
		return m_swapchainRenderPass;
	}

	void RemoveSwapchainExternalRenderPass() noexcept
	{
		m_swapchainRenderPass.reset();
	}

	[[nodiscard]]
	size_t GetActiveRenderPassCount() const noexcept
	{
		size_t activeRenderPassCount = m_renderPasses.GetIndicesManager().GetActiveIndexCount();

		if (m_swapchainRenderPass)
			++activeRenderPassCount;

		return activeRenderPassCount;
	}

protected:
	void Update(VkDeviceSize frameIndex) const noexcept
	{
		m_cameraManager.Update(frameIndex);

		static_cast<Derived const*>(this)->_updatePerFrame(frameIndex);

		m_externalResourceManager->UpdateExtensionData(static_cast<size_t>(frameIndex));
	}

	void CreateGraphicsPipelineLayout()
	{
		if (!std::empty(m_graphicsDescriptorBuffers))
			m_graphicsPipelineLayout.Create(m_graphicsDescriptorBuffers.front().GetValidLayouts());

		m_graphicsPipelineManager.SetPipelineLayout(m_graphicsPipelineLayout.Get());
	}

	void ResetGraphicsPipeline()
	{
		CreateGraphicsPipelineLayout();

		m_graphicsPipelineManager.RecreateAllGraphicsPipelines();
	}

	void _setShaderPath(const std::wstring& shaderPath)
	{
		m_graphicsPipelineManager.SetShaderPath(shaderPath);
	}

	void SetCommonGraphicsDescriptorBufferLayout(
		VkShaderStageFlags cameraShaderStage
	) noexcept {
		m_cameraManager.SetDescriptorBufferLayoutGraphics(
			m_graphicsDescriptorBuffers, Derived::s_cameraBindingSlot,
			s_vertexShaderSetLayoutIndex, cameraShaderStage
		);
	}

private:
	[[nodiscard]]
	static ModelBuffers CreateModelBuffers(
		const VkDeviceManager& deviceManager, MemoryManager* memoryManager, size_t frameCount
	) {
		return ModelBuffers{
			deviceManager.GetLogicalDevice(), memoryManager,
			static_cast<std::uint32_t>(frameCount),
			Derived::GetModelBuffersQueueFamilies(deviceManager)
		};
	}

protected:
	std::unique_ptr<ModelManager_t>        m_modelManager;
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
