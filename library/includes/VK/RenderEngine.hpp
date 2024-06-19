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

/*
class RenderEngine {
public:
	RenderEngine() noexcept;
	virtual ~RenderEngine() = default;

	virtual void ExecutePreRenderStage(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) = 0;
	virtual void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) = 0;
	virtual void Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) = 0;
	virtual void ExecutePostRenderStage() = 0;
	virtual void ConstructPipelines() = 0;
	virtual void UpdateModelBuffers(VkDeviceSize frameIndex) const noexcept = 0;

	virtual void ResizeViewportAndScissor(
		std::uint32_t width, std::uint32_t height
	) noexcept = 0;

	virtual void AddGVerticesAndIndices(
		VkDevice device, std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
	) noexcept = 0;
	virtual void RecordModelDataSet(
		const std::vector<std::shared_ptr<Model>>& models, const std::wstring& fragmentShader
	) noexcept = 0;
	virtual void AddMeshletModelSet(
		std::vector<MeshletModel>& meshletModels, const std::wstring& fragmentShader
	) noexcept = 0;
	virtual void AddGVerticesAndPrimIndices(
		VkDevice device, std::vector<Vertex>&& gVertices,
		std::vector<std::uint32_t>&& gVerticesIndices, std::vector<std::uint32_t>&& gPrimIndices
	) noexcept = 0;

	virtual void BindResourcesToMemory(VkDevice device) = 0;
	virtual void RecordCopy(VkCommandBuffer transferBuffer) noexcept = 0;
	virtual void ReleaseUploadResources() noexcept = 0;
	virtual void AcquireOwnerShipGraphics(VkCommandBuffer graphicsCmdBuffer) noexcept = 0;
	virtual void ReleaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept = 0;

	virtual void CreateBuffers(VkDevice device) noexcept;
	virtual void CopyData() noexcept;
	virtual void AcquireOwnerShipCompute(VkCommandBuffer computeCmdBuffer) noexcept;

	void SetShaderPath(const wchar_t* path) noexcept;

protected:
	std::wstring m_shaderPath;
};
*/

class RenderEngine
{
public:
	RenderEngine(
		VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
		VkQueueFamilyMananger const* queueFamilyManager, std::shared_ptr<ThreadPool> threadPool,
		size_t frameCount
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
	size_t AddTextureAsCombined(std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height);
	[[nodiscard]]
	size_t AddTextureAsCombined(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height,
		size_t samplerIndex
	);

	void UnbindCombinedTexture(size_t index);
	void UnbindCombinedTexture(size_t textureIndex, size_t samplerIndex);
	void BindCombinedTexture(size_t index);
	void BindCombinedTexture(size_t textureIndex, size_t samplerIndex);

	void RemoveTexture(size_t index);

protected:
	[[nodiscard]]
	virtual std::uint32_t GetCombinedTextureBindingSlot() const noexcept = 0;
	[[nodiscard]]
	virtual std::uint32_t GetSampledTextureBindingSlot() const noexcept = 0;
	[[nodiscard]]
	virtual std::uint32_t GetSamplerBindingSlot() const noexcept = 0;

protected:
	std::shared_ptr<ThreadPool>     m_threadPool;
	MemoryManager                   m_memoryManager;
	VkCommandQueue                  m_graphicsQueue;
	VkCommandQueue                  m_transferQueue;
	StagingBufferManager            m_stagingManager;
	std::vector<VkDescriptorBuffer> m_graphicsDescriptorBuffers;
	TextureStorage                  m_textureStorage;
	TextureManager                  m_textureManager;
	MaterialBuffers                 m_materialBuffers;
	CameraManager                   m_cameraManager;
	DepthBuffer                     m_depthBuffers;
	VKRenderPass                    m_renderPass;
	VkClearColorValue               m_backgroundColour;

public:
	RenderEngine(const RenderEngine&) = delete;
	RenderEngine& operator=(const RenderEngine&) = delete;

	RenderEngine(RenderEngine&& other) noexcept
		: m_threadPool{ std::move(other.m_threadPool) },
		m_memoryManager{ std::move(other.m_memoryManager) },
		m_graphicsQueue{ std::move(other.m_graphicsQueue) },
		m_transferQueue{ std::move(other.m_transferQueue) },
		m_stagingManager{ std::move(other.m_stagingManager) },
		m_graphicsDescriptorBuffers{ std::move(other.m_graphicsDescriptorBuffers) },
		m_textureStorage{ std::move(other.m_textureStorage) },
		m_textureManager{ std::move(other.m_textureManager) },
		m_materialBuffers{ std::move(other.m_materialBuffers) },
		m_cameraManager{ std::move(other.m_cameraManager) },
		m_depthBuffers{ std::move(other.m_depthBuffers) },
		m_renderPass{ std::move(other.m_renderPass) },
		m_backgroundColour{ other.m_backgroundColour }
	{}
	RenderEngine& operator=(RenderEngine&& other) noexcept
	{
		m_threadPool                = std::move(other.m_threadPool);
		m_memoryManager             = std::move(other.m_memoryManager);
		m_graphicsQueue             = std::move(other.m_graphicsQueue);
		m_transferQueue             = std::move(other.m_transferQueue);
		m_stagingManager            = std::move(other.m_stagingManager);
		m_graphicsDescriptorBuffers = std::move(other.m_graphicsDescriptorBuffers);
		m_textureStorage            = std::move(other.m_textureStorage);
		m_textureManager            = std::move(other.m_textureManager);
		m_materialBuffers           = std::move(other.m_materialBuffers);
		m_cameraManager             = std::move(other.m_cameraManager);
		m_depthBuffers              = std::move(other.m_depthBuffers);
		m_renderPass                = std::move(other.m_renderPass);
		m_backgroundColour          = other.m_backgroundColour;

		return *this;
	}
};
#endif
