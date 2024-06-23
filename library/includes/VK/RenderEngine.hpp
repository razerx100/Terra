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
#include <SwapchainManager.hpp>

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

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera> camera) noexcept
	{
		return m_cameraManager.AddCamera(std::move(camera));
	}
	void SetCamera(std::uint32_t index) noexcept { m_cameraManager.SetCamera(index); }
	void RemoveCamera(std::uint32_t index) noexcept { m_cameraManager.RemoveCamera(index); }

	void WaitForQueues() { m_graphicsQueue.WaitForQueueToFinish(); }

	virtual void SetDeviceExtensions(VkDeviceManager& deviceManager) noexcept;
	virtual void SetShaderPath(const std::wstring& shaderPath) noexcept = 0;
	virtual void AddFragmentShader(const std::wstring& fragmentShader) = 0;
	virtual void ChangeFragmentShader(
		std::uint32_t modelBundleID, const std::wstring& fragmentShader
	) = 0;

	void BeginRenderPass(
		size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea
	);
	virtual void Render(VkDeviceSize frameIndex) = 0;
	virtual void Resize(
		std::uint32_t width, std::uint32_t height,
		bool hasSwapchainFormatChanged, VkFormat swapchainFormat
	);

	[[nodiscard]]
	virtual std::uint32_t AddModel(
		std::shared_ptr<ModelVS>&& model, const std::wstring& fragmentShader
	);
	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelVS>>&& modelBundle, const std::wstring& fragmentShader
	);
	[[nodiscard]]
	virtual std::uint32_t AddModel(
		std::shared_ptr<ModelMS>&& model, const std::wstring& fragmentShader
	);
	[[nodiscard]]
	virtual std::uint32_t AddModelBundle(
		std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const std::wstring& fragmentShader
	);

	virtual void RemoveModelBundle(std::uint32_t bundleID) noexcept = 0;

	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleVS> meshBundle);
	[[nodiscard]]
	virtual std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle);

	virtual void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept = 0;

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
		m_viewportAndScissors{ other.m_viewportAndScissors }
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

		return *this;
	}
};
#endif
